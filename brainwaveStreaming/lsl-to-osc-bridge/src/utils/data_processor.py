"""
This module contains the data processing logic for the LSL-to-OSC bridge.
"""

from brainflow.data_filter import DataFilter, WindowOperations, DetrendOperations
from brainflow.ml_model import (
    MLModel,
    BrainFlowMetrics,
    BrainFlowClassifiers,
    BrainFlowModelParams,
)
import numpy as np


def filter_data(data):
    """
    Implement filtering logic here
    """
    return data


def scale_data(data, scale_factor):
    """
    Scale the data by the given factor
    """
    return data * scale_factor


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
    data_copy = np.array(data)

    # Apply linear detrending
    DataFilter.detrend(data_copy, DetrendOperations.LINEAR.value)

    # Calculate FFT size (nearest power of 2)
    nfft = DataFilter.get_nearest_power_of_two(sampling_rate)

    # Calculate PSD using Welch's method
    psd = DataFilter.get_psd_welch(
        data_copy,
        nfft,
        nfft // 2,
        sampling_rate,
        WindowOperations.BLACKMAN_HARRIS.value,
    )

    # Extract band power for the specified frequency range
    band_power = DataFilter.get_band_power(psd, band[0], band[1])

    return band_power


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
    # Reshape data for BrainFlow's get_avg_band_powers function
    # BrainFlow expects data in shape [channels x samples]
    data_reshaped = np.array([data])

    # Get average band powers - this returns a tuple (feature_vector, psd_values)
    bands = DataFilter.get_avg_band_powers(data_reshaped, [0], sampling_rate, True)
    feature_vector = bands[0]

    metrics = {}

    # Calculate mindfulness (focus) score
    try:
        mindfulness_params = BrainFlowModelParams(
            BrainFlowMetrics.MINDFULNESS.value,
            BrainFlowClassifiers.DEFAULT_CLASSIFIER.value,
        )
        mindfulness = MLModel(mindfulness_params)
        mindfulness.prepare()
        metrics["mindfulness"] = mindfulness.predict(feature_vector)
        mindfulness.release()
    except Exception as e:
        print(f"Error calculating mindfulness: {e}")
        metrics["mindfulness"] = 0

    # Calculate restfulness (relaxation) score
    try:
        restfulness_params = BrainFlowModelParams(
            BrainFlowMetrics.RESTFULNESS.value,
            BrainFlowClassifiers.DEFAULT_CLASSIFIER.value,
        )
        restfulness = MLModel(restfulness_params)
        restfulness.prepare()
        metrics["restfulness"] = restfulness.predict(feature_vector)
        restfulness.release()
    except Exception as e:
        print(f"Error calculating restfulness: {e}")
        metrics["restfulness"] = 0

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
        return result, extras

    return result
