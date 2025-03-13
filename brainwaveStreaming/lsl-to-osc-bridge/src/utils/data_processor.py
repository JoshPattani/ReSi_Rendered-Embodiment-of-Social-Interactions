"""
This module contains the data processing logic for the LSL-to-OSC bridge.
"""

from brainflow.data_filter import (
    DataFilter,
    FilterTypes,
    AggOperations,
    NoiseTypes,
    WindowOperations,
    DetrendOperations,
)
from brainflow.ml_model import (
    MLModel,
    BrainFlowMetrics,
    BrainFlowClassifiers,
    BrainFlowModelParams,
)
import time

import matplotlib
import numpy as np
import pandas as pd

matplotlib.use("Agg")
import matplotlib.pyplot as plt


def check_eeg_quality(data):
    """
    Check the quality of EEG data.

    Args:
        data (array): EEG data array

    Returns:
        dict: Quality metrics
    """
    metrics = {}

    # First check if there's actual data (not zeros)
    if np.all(np.array(data) == 0):
        metrics["flatline"] = True
        metrics["quality"] = "inactive"
        return metrics

    # Check for flatlines - using a more lenient threshold
    if np.std(data) < 0.005:  # More permissive threshold
        metrics["flatline"] = True
        metrics["quality"] = "poor"
        print("Flatline detected")
        return metrics

    # Check for extreme values - using a higher threshold
    if np.max(np.abs(data)) > 1000000:  # Higher threshold for extreme values
        metrics["extreme_values"] = True
        metrics["quality"] = "poor"
        print("Extreme values detected")
        return metrics

    # Good quality if we pass all checks
    metrics["quality"] = "good"
    return metrics


def filter_data(
    data,
    sampling_rate=250,
    filter_type="bandpass",
    low_cutoff=1.0,
    high_cutoff=50.0,
    order=3,
    notch=True,
):
    """
    Filter EEG data using BrainFlow filters.

    Args:
        data (array): EEG data array to filter
        sampling_rate (int): Sampling rate of the data in Hz
        filter_type (str): Type of filter to apply ('bandpass', 'lowpass', 'highpass', 'notch')
        low_cutoff (float): Low cutoff frequency for bandpass/highpass filter
        high_cutoff (float): High cutoff frequency for bandpass/lowpass filter
        order (int): Filter order
        notch (bool): Whether to apply a notch filter to remove power line interference

    Returns:
        array: Filtered data
    """
    # Make a copy to avoid modifying original data
    data_copy = np.array(data, dtype=np.float64)

    # Apply detrending to remove DC offset and linear trends
    DataFilter.detrend(data_copy, DetrendOperations.LINEAR.value)

    # Apply bandpass filter to remove noise
    if filter_type == "bandpass":
        DataFilter.perform_bandpass(
            data_copy,
            sampling_rate,
            low_cutoff,
            high_cutoff,
            order,
            FilterTypes.BUTTERWORTH.value,
            0,
        )
    elif filter_type == "lowpass":
        DataFilter.perform_lowpass(
            data_copy,
            sampling_rate,
            high_cutoff,
            order,
            FilterTypes.BUTTERWORTH.value,
            0,
        )
    elif filter_type == "highpass":
        DataFilter.perform_highpass(
            data_copy,
            sampling_rate,
            low_cutoff,
            order,
            FilterTypes.BUTTERWORTH.value,
            0,
        )

    # Apply notch filter to remove power line interference (50/60 Hz)
    if notch:
        # Detect if we're more likely dealing with 50Hz (Europe/Asia) or 60Hz (Americas)
        # Default to 60Hz
        line_freq = 60.0
        DataFilter.remove_environmental_noise(
            data_copy, sampling_rate, NoiseTypes.FIFTY_AND_SIXTY.value
        )

    return data_copy


def scale_data(data, scale_factor=1.0, normalize=False):
    """
    Scale the data by the given factor, with optional normalization.

    Args:
        data (array): Data to scale
        scale_factor (float): Factor to scale data by
        normalize (bool): Whether to normalize data to range [0,1] or [-1,1]

    Returns:
        array: Scaled data
    """
    # Make a copy to avoid modifying original data
    data_copy = np.array(data)

    if normalize:
        # Z-score normalization
        mean = np.mean(data_copy)
        std = np.std(data_copy)
        if std != 0:
            data_copy = (data_copy - mean) / std

    # Apply scaling factor
    return data_copy * scale_factor


def bandPower(data, band, sampling_rate):
    """
    Calculate power in a specific frequency band using Welch's method.

    Args:
        data (array): EEG data array for a single channel
        band (tuple): Tuple containing (low_freq, high_freq) defining the frequency band
        sampling_rate (int): Sampling rate of the data in Hz

    Returns:
        float: Power in the specified frequency band
    """
    # Make a copy to avoid modifying original data
    data_copy = np.array(data, dtype=np.float64)

    data_len = len(data_copy)

    if data_len < 10:
        print(
            f"Warning: Data length too small ({data_len} samples). Cannot calculate band power."
        )
        return 0.0

    # Apply linear detrending
    DataFilter.detrend(data_copy, DetrendOperations.LINEAR.value)

    try:
        # Calculate appropriate FFT size - CRITICAL FIX
        # nfft must be SMALLER than data length
        if data_len <= 256:
            # For small data segments, use a smaller nfft
            nfft = min(128, data_len - 1)
        else:
            # For larger data, use a power of 2 but ensure it's smaller than data length
            nfft = min(DataFilter.get_nearest_power_of_two(sampling_rate), data_len - 1)

        # Further ensure nfft is valid
        if nfft <= 0 or nfft >= data_len:
            print(f"Error: Invalid nfft={nfft} for data_len={data_len}")
            return 0.0

        # Calculate PSD using Welch's method
        # get_psd_welch returns a tuple of (frequencies, psd_values)
        psd_data = DataFilter.get_psd_welch(
            data_copy,
            nfft,
            nfft // 2,
            sampling_rate,
            WindowOperations.BLACKMAN_HARRIS.value,
        )

        # Validate frequency range
        freqs = psd_data[0]  # Frequencies are in the first element of the tuple

        # Ensure band limits are within available frequency range
        low_freq = min(band[0], band[1])
        high_freq = max(band[0], band[1])

        # Ensure band limits are within available frequency range
        low_freq = max(low_freq, freqs[0])
        high_freq = min(high_freq, freqs[-1])

        if low_freq >= high_freq:
            print(f"Warning: Invalid frequency range [{low_freq}, {high_freq}]")
            return 0.0

        # Extract band power for the specified frequency range
        band_power = DataFilter.get_band_power(psd_data, low_freq, high_freq)
        return band_power if band_power > 0 else 0.0
    except Exception as e:
        print(f"Error calculating band power: {e}")
        # Return a default value if calculation fails
        return 0.0


def calculate_band_powers(data, sampling_rate):
    """
    Calculate power in standard EEG frequency bands.

    Args:
        data (array): EEG data array for a single channel
        sampling_rate (int): Sampling rate of the data in Hz

    Returns:
        dict: Power values for each frequency band and relevant ratios
    """
    # Standard EEG frequency bands
    bands = {
        "delta": (1.0, 4.0),
        "theta": (4.0, 7.0),
        "alpha": (7.0, 13.0),
        "beta": (14.0, 30.0),
        "gamma": (30.0, 100.0),
    }

    powers = {}
    for band_name, band_range in bands.items():
        powers[band_name] = bandPower(data, band_range, sampling_rate)

    # Calculate useful ratios
    if powers["beta"] != 0:
        powers["alpha/beta"] = powers["alpha"] / powers["beta"]
        powers["theta/beta"] = powers["theta"] / powers["beta"]
    else:
        powers["alpha/beta"] = 0
        powers["theta/beta"] = 0

    return powers


def transform_data(data, transformation_function):
    """
    Apply a custom transformation to the data
    """
    return transformation_function(data)


def eeg_metrics(data, sampling_rate):
    """
    Calculate EEG metrics for focus (mindfulness) and relaxation.

    Args:
        data (array): EEG data array for a single channel
        sampling_rate (int): Sampling rate of the data in Hz

    Returns:
        dict: Dictionary containing mindfulness and restfulness scores
    """
    metrics = {"mindfulness": 0.0, "restfulness": 0.0}

    # Check if there's enough data
    min_length = sampling_rate * 4  # At least 4 seconds of data
    if len(data) < min_length:
        print(
            f"Warning: Not enough data for metrics calculation ({len(data)} < {min_length}). Padding data."
        )
        # Pad the data by repeating it
        repeats_needed = int(np.ceil(min_length / len(data)))
        data = np.tile(data, repeats_needed)[:min_length]

    # Reshape data for BrainFlow's get_avg_band_powers function
    # BrainFlow expects data in shape [channels x samples]
    # data_reshaped = np.array([data])

    # # Get average band powers - this returns a tuple (feature_vector, psd_values)
    # bands = DataFilter.get_avg_band_powers(data_reshaped, [0], sampling_rate, True)
    # feature_vector = bands[0]

    # metrics = {}
    try:
        # Instead of using get_avg_band_powers directly, calculate band powers manually
        # This gives us more control over the calculation
        band_powers = calculate_band_powers(data, sampling_rate)

        # Create feature vector for ML models (format expected by BrainFlow)
        # Standard order: delta, theta, alpha, beta, gamma
        feature_vector = np.array(
            [
                band_powers["delta"],
                band_powers["theta"],
                band_powers["alpha"],
                band_powers["beta"],
                band_powers["gamma"],
            ]
        )

        # Normalize feature vector to sum to 1 (relative power)
        sum_powers = np.sum(feature_vector)
        if sum_powers > 0:
            feature_vector = feature_vector / sum_powers

        # Create a BrainFlow ML model instance
        mindfulness_params = BrainFlowModelParams(
            BrainFlowMetrics.MINDFULNESS.value,
            BrainFlowClassifiers.DEFAULT_CLASSIFIER.value,
        )
        mindfulness = MLModel(mindfulness_params)

        # Prepare the model and get predictions
        mindfulness.prepare()
        focus = mindfulness.predict(feature_vector)
        print("Mindfulness: %s" % str(focus))
        mindfulness.release()

        # Create a BrainFlow ML model instance
        restfulness_params = BrainFlowModelParams(
            BrainFlowMetrics.RESTFULNESS.value,
            BrainFlowClassifiers.DEFAULT_CLASSIFIER.value,
        )
        restfulness = MLModel(restfulness_params)

        # Prepare the model and get predictions
        restfulness.prepare()
        relax = restfulness.predict(feature_vector)
        print("Restfulness: %s" % str(relax))
        restfulness.release()

        # Update metrics with the calculated values
        metrics["mindfulness"] = focus
        metrics["restfulness"] = relax

    except Exception as e:
        print(f"Error in eeg_metrics: {e}")
        # Return default values if calculation fails
        return metrics

    return metrics


def process_data(
    lsl_data,
    scale_factor=1.0,
    transformation_function=None,
    calculate_bands=False,
    calculate_metrics=False,
    sampling_rate=None,
):
    """
    Process the incoming LSL data.

    Args:
        lsl_data: Data from LSL stream
        scale_factor: Factor to scale data by
        transformation_function: Optional function to transform data
        calculate_bands: Whether to calculate EEG band powers
        calculate_metrics: Whether to calculate EEG metrics (focus/relaxation)
        sampling_rate: Sampling rate of the data (required if calculate_bands=True)

    Returns:
        Processed data and optional band powers and metrics
    """
    filtered_data = filter_data(lsl_data)
    scaled_data = scale_data(filtered_data, scale_factor)

    result = scaled_data

    if transformation_function:
        result = transform_data(scaled_data, transformation_function)

    response = result
    extras = {}

    if calculate_bands or calculate_metrics:
        if sampling_rate is None:
            raise ValueError(
                "sampling_rate must be provided when calculate_bands=True or calculate_metrics=True"
            )

    if calculate_bands:
        extras["band_powers"] = calculate_band_powers(result, sampling_rate)

    if calculate_metrics:
        extras["metrics"] = eeg_metrics(result, sampling_rate)

    if extras:
        return response, extras

    return response
