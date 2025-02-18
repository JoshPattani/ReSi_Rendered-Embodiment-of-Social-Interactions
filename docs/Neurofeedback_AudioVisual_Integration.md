# Neurofeedback_AudioVisual_Integration.md

## 1. Introduction

This document outlines how biometric data—particularly EEG, PPG, GSR, and Temperature—are transformed into real-time audio and visual experiences within the RESI installation. The primary goal is to create an immersive and interactive environment where participants’ physiological states shape the audiovisual output, promoting mindfulness and collaborative engagement.

---

## 2. Data Pipeline Overview

1. **Sensors**: EEG, PPG, GSR, and Temperature data are captured through wearable hardware (e.g., OpenBCI Cyton or similar devices).
2. **Data Aggregation**: All data streams funnel into a real-time collection system such as LabstreamingLayer (LSL).
3. **Signal Processing**: Software modules apply filtering, artifact rejection, and analysis (e.g., bandpower for EEG, heart rate variability for PPG, and temperature normalization).
4. **OSC Conversion**: Processed metrics (like alpha amplitude or coherence) are mapped to OSC messages for easy integration into audiovisual platforms (~~TouchDesigner~~, Max/MSP, or similar).
5. **Audiovisual Rendering**: Output parameters drive generative visuals, soundscapes, and interactive elements, providing immediate feedback to participants.

---

## 3. EEG Data Integration

- **Signal Pre-processing**
  - Apply bandpass filters (e.g., 1–50 Hz) to remove noise and normalize signals.
- **Feature Extraction**
  - Bandpower in alpha (8–12 Hz), beta (12–30 Hz), theta (4–8 Hz), and gamma (30–50 Hz) ranges.
  - Coherence or synchrony metrics for multi-participant scenarios (e.g., cross-spectral coherence).
- **Neurofeedback Mapping**
  - **Visual**: Change color gradients, particle density, or wave patterns based on alpha or beta power.
  - **Audio**: Manipulate volume, pitch shifts, or layered effects tied to shifting EEG bands.

---

## 4. PPG, GSR, and Temperature Data Integration

- **PPG**

  - Extract heart rate and heart rate variability (HRV).
  - Use HRV to alter slower-evolving parameters (e.g., scene transitions or ambient drone layers).
  - Track cardiac rhythms; might be useful for deeper, rhythmic elements like pulse-driven bass or background oscillations.

- **GSR**

  - Map galvanic skin response changes to short, punctuated events (e.g., particle bursts or subtle lighting shifts).

- **Temperature**

  - Normalize temperature data to account for environmental factors.
  - Use temperature shifts to modulate color temperature, ambient lighting, or granular synthesis parameters.

- **~~ECG~~**
  - ~~Track precise cardiac cycles; useful for deeper, rhythmic elements like pulse-driven bass or background oscillations.~~
  - ~~Utilize ECG data to enhance synchronization with audio elements and create immersive experiences.~~

---

## 5. Audiovisual Design Guidelines

1. **Real-Time Adaptation**: Ensure minimal latency (preferably under 100 ms) for an immediate feedback loop.
2. **Dynamic Range**: Provide smooth gradients for subtle changes in biometric data while still allowing dramatic shifts to heighten engagement.
3. **Aesthetic Unity**: Keep color palettes, sonic textures, and motion patterns cohesive and thematically consistent with the installation’s mindfulness objective.

---

## 6. Implementation Workflow

1. **Acquire and Filter Data**
   - Load raw sensor data into the main pipeline via LSL or a similar tool.
2. **Process & Analyze**
   - Use frameworks like Python (NumPy, MNE) or custom Max/MSP patches for real-time calculations.
3. **Convert to Control Signals**
   - Map processed metrics to OSC or MIDI messages for cross-platform use.
4. **Visual Synthesis**
   - In ~~TouchDesigner~~ Max 9: attach signals to particle systems, fluid simulations, or generative shaders.
   - In Unity or Unreal Engine: apply signal-driven parameters to materials, lighting, or physics systems.
5. **Audio Synthesis**
   - In Max/MSP: control synthesis parameters, filter cutoff, or generative sequences with biometric values.
   - In Ableton Live: use MIDI to modulate reverb, delay, or granular synthesis for immersive soundscapes.

---

## 7. System Testing and Calibration

- **Baseline and Artifact Checks**
  - Regularly check sensor data against known baselines (e.g., resting alpha amplitude).
  - Validate latency using test signals (e.g., a pulse simulator).
- **Iterative Testing**
  - Perform trials with small groups to optimize user experience and signal stability.
  - Adjust mapping strategies based on participant feedback.

---

## 8. Conclusion

An effective neurofeedback integration depends on stable data capture, thoughtful signal processing, and aesthetically coherent mapping to real-time audiovisual elements. Refer to companion documents (e.g., _Installation_Environment_Setup.docx_ and _Hardware Specifications_) for details on physical layout and hardware maintenance.
