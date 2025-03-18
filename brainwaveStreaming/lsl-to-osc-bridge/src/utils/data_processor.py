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

# Constants
focusCurrent = 0.0
focusMin = 0.0
focusMax = 0.0

relaxCurrent = 0.0
relaxMin = 0.0
relaxMax = 0.0

calibrated = False


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
    low_cutoff=2.0,
    high_cutoff=50.0,
    order=4,
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
    # Check data dimensionality
    data_array = np.array(data)
    original_shape = data_array.shape
    print(f"Filter input shape: {original_shape}")

    # Handle different data shapes
    if len(original_shape) == 1:
        # Already 1D array (single channel)
        channels = [data_array]
    elif len(original_shape) == 2:
        # 2D array - can be [samples, channels] or [channels, samples]
        # For LSL data, it's typically [samples, channels]
        # Convert to list of 1D arrays (one per channel)
        channels = [data_array[:, i] for i in range(original_shape[1])]
    else:
        raise ValueError(f"Unsupported data shape: {original_shape}")

    # Process each channel separately
    filtered_channels = []
    for i, channel_data in enumerate(channels):
        # Make a copy to avoid modifying original data
        data_copy = np.array(channel_data, dtype=np.float64)
        # Apply bandpass filter to remove noise
        if filter_type == "bandpass":
            DataFilter.perform_bandpass(
                data_copy,
                sampling_rate,
                low_cutoff,
                high_cutoff,
                order,
                FilterTypes.BESSEL_ZERO_PHASE.value,
                0,
            )
        elif filter_type == "lowpass":
            DataFilter.perform_lowpass(
                data_copy,
                sampling_rate,
                high_cutoff,
                5,
                FilterTypes.CHEBYSHEV_TYPE_1_ZERO_PHASE.value,
                1,
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

        # Append the filtered channel data
        filtered_channels.append(data_copy)

    # Reconstruct the data into its original shape
    if len(original_shape) == 1:
        return filtered_channels[0]
    else:
        return np.column_stack(filtered_channels)


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
    data_copy = np.array(data, dtype=np.float64).flatten()  # Ensure 1D array

    data_len = len(data_copy)

    # Check minimum data requirements (at least 1 second of data)
    # BrainFlow needs enough data for reliable spectral analysis
    min_samples_needed = sampling_rate

    if data_len < min_samples_needed:
        print(f"Data too short ({data_len}/{min_samples_needed} samples) - padding")
        # Pad by repeating the data to reach minimum length
        repeats_needed = int(np.ceil(min_samples_needed / data_len))
        data_copy = np.tile(data_copy, repeats_needed)[:min_samples_needed]
        data_len = len(data_copy)

    # Apply linear detrending
    DataFilter.detrend(data_copy, DetrendOperations.LINEAR.value)

    try:
        # Calculate appropriate FFT size for spectrum analysis
        # For short data segments, use smaller nfft
        if data_len <= 256:
            # Use simpler calculations for short data
            nfft = max(64, 2 ** int(np.log2(data_len / 4)))
            overlap = nfft // 4  # Less overlap for stability
            window = WindowOperations.HANNING.value  # More stable window
        else:
            # For longer data use optimal FFT size
            nfft = min(512, DataFilter.get_nearest_power_of_two(sampling_rate))
            overlap = nfft // 2
            window = WindowOperations.BLACKMAN_HARRIS.value

        # Sanity checks
        if nfft <= 0 or nfft >= data_len:
            nfft = max(32, data_len // 2)

        # Print diagnostics for troubleshooting
        # print(f"PSD params: data_len={data_len}, nfft={nfft}, overlap={overlap}")

        # Calculate PSD using Welch's method
        # get_psd_welch returns a tuple of (frequencies, psd_values)
        psd_data = DataFilter.get_psd_welch(
            data_copy,
            nfft,
            overlap,
            sampling_rate,
            WindowOperations.BLACKMAN_HARRIS.value,
        )

        # print(f"PSD data: {psd_data}")
        if not psd_data:
            print("Error: No PSD data returned")

        # Find low and high frequency limits for the specified band
        low_freq, high_freq = band

        if low_freq >= high_freq:
            print(f"Warning: Invalid frequency range [{low_freq}, {high_freq}]")
            return 0.0

        # Extract band power for the specified frequency range
        band_power = DataFilter.get_band_power(psd_data, low_freq, high_freq)
        return band_power if band_power > 0 else 0.0
    except Exception as e:
        print(f"Error calculating band power: {e}")
        # For error cases, use a fallback method or default value

        # Fallback: use simple bandpower estimate based on filtered data
        # This is less accurate but better than returning zero
        try:
            # Simple bandpower: apply bandpass filter and calculate signal power
            data_filtered = data_copy.copy()

            # Bandpass filter to isolate the frequency band
            DataFilter.perform_bandpass(
                data_filtered,
                sampling_rate,
                max(0.5, band[0]),  # Ensure lower bound is at least 0.5 Hz
                min(band[1], sampling_rate / 2 - 1),  # Below Nyquist
                3,  # Lower order for stability
                FilterTypes.BUTTERWORTH.value,
                0,
            )

            # Calculate power as mean squared amplitude
            return np.mean(data_filtered**2)
        except:
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
        "delta": (0.5, 4.0),
        "theta": (4.0, 8.0),
        "alpha": (8.0, 13.0),
        "beta": (13.0, 32.0),
        "gamma": (32.0, 100.0),
    }

    # Check if we have 1D or 2D data
    data_array = np.array(data)
    data_shape = data_array.shape

    if len(data_shape) == 1:
        # Single channel data - process directly
        return _calculate_single_channel_powers(data_array, bands, sampling_rate)
    elif len(data_shape) == 2:
        # Multi-channel data [samples, channels]
        print(f"Processing multi-channel data with shape {data_shape}")
        channels_count = data_shape[1]
        all_channel_powers = []

        # Calculate powers for each channel
        for ch in range(channels_count):
            channel_data = data_array[:, ch]
            channel_powers = _calculate_single_channel_powers(
                channel_data, bands, sampling_rate
            )
            all_channel_powers.append(channel_powers)

        # Average powers across all channels
        avg_powers = {}
        for band_name in bands.keys():
            avg_powers[band_name] = np.mean(
                [ch_power[band_name] for ch_power in all_channel_powers]
            )

        # Calculate ratios from averages
        # if avg_powers["beta"] != 0:
        #     avg_powers["alpha/beta"] = avg_powers["alpha"] / avg_powers["beta"]
        #     avg_powers["theta/beta"] = avg_powers["theta"] / avg_powers["beta"]
        # else:
        #     avg_powers["alpha/beta"] = 0
        #     avg_powers["theta/beta"] = 0

        return avg_powers
    else:
        raise ValueError(f"Unsupported data shape: {data_shape}")


def _calculate_single_channel_powers(channel_data, bands, sampling_rate):
    """Helper function to calculate powers for a single channel"""
    powers = {}
    for band_name, band_range in bands.items():
        powers[band_name] = bandPower(channel_data, band_range, sampling_rate)

    # Calculate useful ratios
    # if powers["beta"] != 0:
    #     powers["alpha/beta"] = powers["alpha"] / powers["beta"]
    #     powers["theta/beta"] = powers["theta"] / powers["beta"]
    # else:
    #     powers["alpha/beta"] = 0
    #     powers["theta/beta"] = 0

    return powers


def transform_data(data, transformation_function):
    """
    Apply a custom transformation to the data
    """
    return transformation_function(data)


# simple hash function to track changes in data
def hash_data(data):
    """Generate a simple hash of the data to detect changes"""
    if isinstance(data, np.ndarray):
        return hash(np.sum(data) + np.mean(data))
    return 0


def eeg_metrics(data, eeg_channels, sampling_rate):
    """
    Calculate EEG metrics for focus (mindfulness) and relaxation.

    Args:
        data (array): EEG data array for a single channel
        sampling_rate (int): Sampling rate of the data in Hz

    Returns:
        dict: Dictionary containing mindfulness and restfulness scores
    """
    global calibrated, focusMin, focusMax, relaxMin, relaxMax

    # Add a data hash to check for updates
    data_hash = hash_data(data)
    print(f"Data hash: {data_hash}")

    # Add timestamp for uniqueness
    timestamp = time.time()

    metrics = {
        "mindfulness": 0.0,
        "focusMin": focusMin,
        "focusMax": focusMax,
        "restfulness": 0.0,
        "relaxMin": relaxMin,
        "relaxMax": relaxMax,
        "timestamp": timestamp,
        "avg_band_powers": {},
    }

    try:
        # Check if we have 1D or 2D data
        data_array = np.array(data)
        data_shape = data_array.shape

        # Calculate band powers manually since that's working
        band_powers = calculate_band_powers(data_array, sampling_rate)
        metrics["avg_band_powers"] = band_powers

        # Create feature vector in format BrainFlow expects (5 values: delta, theta, alpha, beta, gamma)
        feature_vector = np.array(
            [
                band_powers["delta"],
                band_powers["theta"],
                band_powers["alpha"],
                band_powers["beta"],
                band_powers["gamma"],
            ]
        )

        # Normalize to sum to 1.0 (important for BrainFlow's models)
        feature_sum = np.sum(feature_vector)
        if feature_sum > 0:
            feature_vector = feature_vector / feature_sum

        # Calculate mindfulness (focus)
        try:
            mindfulness_params = BrainFlowModelParams(
                BrainFlowMetrics.MINDFULNESS.value,
                BrainFlowClassifiers.DEFAULT_CLASSIFIER.value,
            )
            mindfulness = MLModel(mindfulness_params)
            mindfulness.prepare()
            focus = mindfulness.predict(feature_vector)
            print(f"Mindfulness: {focus} at {timestamp:.2f}")
            mindfulness.release()

            # Convert to scalar if needed
            if hasattr(focus, "item"):
                focus = float(focus.item())

            metrics["mindfulness"] = focus

        except Exception as e:
            print(f"Error calculating mindfulness: {e}")

        # Calculate restfulness (relaxation)
        try:
            restfulness_params = BrainFlowModelParams(
                BrainFlowMetrics.RESTFULNESS.value,
                BrainFlowClassifiers.DEFAULT_CLASSIFIER.value,
            )
            restfulness = MLModel(restfulness_params)
            restfulness.prepare()
            relax = restfulness.predict(feature_vector)
            print(f"Restfulness: {relax} at {timestamp:.2f}")
            restfulness.release()

            # Convert to scalar if needed
            if hasattr(relax, "item"):
                relax = float(relax.item())

            metrics["restfulness"] = relax

        except Exception as e:
            print(f"Error calculating restfulness: {e}")

        if not calibrated:
            focusMin = focus
            focusMax = focus
            relaxMin = relax
            relaxMax = relax
            calibrated = True

        if focus < focusMin:
            focusMin = focus
        if focus > focusMax:
            focusMax = focus
        if relax < relaxMin:
            relaxMin = relax
        if relax > relaxMax:
            relaxMax = relax

        metrics["focusMin"] = focusMin
        metrics["focusMax"] = focusMax
        metrics["relaxMin"] = relaxMin
        metrics["relaxMax"] = relaxMax
    except Exception as e:
        print(f"Error in eeg_metrics: {e}")
        import traceback

        traceback.print_exc()
    return metrics


def process_data(
    lsl_data,
    eeg_channels=4,
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
    # Ensure data is correctly formatted
    data_array = np.array(lsl_data)

    # Calculate a simple data signature to detect changes
    data_signature = np.sum(data_array) / data_array.size if data_array.size > 0 else 0

    # Debug the data shape and signature
    print(f"Data shape: {data_array.shape}, signature: {data_signature:.4f}")

    # Apply filtering to each channel (handled within filter_data)
    filtered_data = filter_data(data_array)
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
        extras["metrics"] = eeg_metrics(result, eeg_channels, sampling_rate)

    if extras:
        return response, extras

    return response
