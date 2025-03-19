# 🚀 RESI: Rendered Embodiment of Social Interaction

🎭 **An immersive neurofeedback-driven interactive installation** where real-time biometric data by two participants at a time fuels **generative audiovisual experiences**. Using EEG, PPG, GSR, and temperature sensors, RESI transforms human physiological states into **soundscapes, visuals, and interactive elements**, bridging neuroscience, art, and technology.

---

## ✨ Project Overview

🔹 **Real-Time Neurofeedback**: Participants' brain activity, heart rate variability, and skin conductivity directly influence the audiovisual environment.  
🔹 **Multi-Sensory Immersion**: Live projections, spatial audio, and adaptive lighting create a responsive, interactive space.  
🔹 **Collaborative & Mindful**: Participants influence each other’s experiences through **inter-brain synchrony** and biometric data correlation.  
🔹 **Dynamic Visualizations**: Immersive graphics that evolve with the synchrony between participants' signals, i.e. **emotional states, attention levels, and physiological responses**.  
🔹 **Multi-Sensory Immersion**: Spatial audio, projection mapping, and adaptive lighting respond to the participants' brain activity and the quality of their social interaction.  
🔹 **Dynamic Visualizations**: Immersive graphics that evolve with the synchrony between participants' signals, i.e. **emotional states, attention levels, and physiological responses**.

RESI is designed to encourage participants to engage in a meaningful and authentic interaction with each other, as the quality of their interaction directly influences the audio-visual experience they have together. With RESI, we create a space for participants to reflect on the nature of human interaction and the role of social connections in shaping our experiences and perceptions of the world. By encouraging self-awareness and connection, the installation aims to foster a deeper understanding of our shared humanity.

### 🎨 **Inspired by**

- **Neural Aesthetics**: Explore the beauty of brainwaves and biometric signals.
- **Human Connection**: Articulating shared experiences through physiological interaction.
- **Interactive Art**: Engaging audiences in a dialogue with technology and self.

---

## 🧠 How It Works

1️⃣ **Sensors Capture Data**

- EEG, PPG, GSR, and temperature readings are acquired via OpenBCI and additional biosensors.

2️⃣ **Processing & Analysis**

- Data is streamed in real-time using LabstreamingLayer (LSL), processed for **bandpower, coherence, HRV, and emotional states**.

3️⃣ **Mapping to OSC/MIDI**

- Biometric data is transformed into control signals for generative audiovisual systems in **Max/MSP, Unity, or custom shaders**.

4️⃣ **Real-Time Audiovisual Synthesis**

- Brainwaves shape particles, heart rate drives bass pulses, and skin conductivity sparks light bursts.

---

## 🔧 Hardware & Software Stack

### 💾 **Hardware**

- OpenBCI Cyton (8-16 channel EEG)
- PPG sensors
- GSR sensors
- ~Temperature sensors~
- High-fidelity speakers (at least 2 channels, 5.1 surround recommended)
- Projector (minimum 1080p resolution, 4K recommended), short-throw lens may be required depending on space

### 🖥 **Software**

- **Arduino IoT Cloud**: Sensor data aggregation
- **OpenSoundControl (OSC)**: Control signal transmission
- **LabstreamingLayer (LSL)**: Real-time biosignal streaming
- **Max/MSP / Ableton Live**: Sound synthesis & modulation
- **Unity / TouchDesigner**: Generative visuals & interaction
- **OpenBCI GUI**: EEG signal processing & visualization

---

## 🎛️ System Architecture

### 📡 **Data Flow**

```mermaid
graph TD;
    A[Sensors] -- Raw Signals --> B[DataProcessing];
    B[DataProcessing] -- Filtered Metrics --> C[OSC_Conversion];
    C -- Control Signals --> D[Visuals];
    C -- Control Signals --> E[Audio];
    D -- Generative Graphics --> Projector;
    E -- Ambient Soundscapes --> Speakers;
```

### 🧬 **Biometric Data Processing**

```mermaid
graph TD;
    flowchart TD
    A[PPG] -->B{Heart Rate} -->|BPM| C[Basic indicator of autonomic activity];
    newLines("`Lower Values [40-60]
    Deep Relaxation/Sleep
    ---
    Mid Values [60-80]
    Resting, Normal State
    ---
    Upper Mid Values [80-100]
    Mild Stress, Cognitive Effort
    ---
    High Values [100+]
    High Stress/Physical Exertion`")
    C --> newLines
    A -->D{Heart Rate Variability}-->|RMSSD| E["`Root Mean Square of Successive Differences (RMSSD) reflects short-term parasympathetic activity.
    ---
    A great marker for relaxation, stress recvery, and overall autonomic balance`"]
    nextLines["`Lower Values (< 20 ms):
    chronic stress, fatigue
    ---
    Lower-mid Values (20-40 ms):
    moderate stress, cognitive effort
    ---
    Mid Values (40-60 ms):
    normal balance, healthy autonomic function
    ---
    Upper Mid Values (60-100 ms):
    relaxed, parasympathetic dominance
    ---
    High Values (>100 ms):
    Deep relaxation, sleep`"]
    E --> nextLines
    F[Cyton]-->G{Band Powers}
    G --> I[Delta]
    G --> J[Theta]
    G --> K[Alpha]
    G --> L[Beta]
    F --> H{Metrics}
    H --> N[Focused]
    H --> O[Relaxed]
    I --> P[0.5 - 4 Hz
    ---
    Linked to deep sleep, unconscious processing]
    J --> Q[4-8 Hz
    ---
    Linked to relaxation, creativity, light sleep]
    K --> R[8-12 Hz
    ---
    Linked to calm focus, meditation, wakeful relaxation]
    L --> S[12-30 Hz
    ---
    Linked to active thinking, problem solving, and stress]
    deltaLines["`Lower Values (< 10 uV^2):
    Fully awake, alert.
    ---
    Lower-mid Values (10 to 50 uV^2):
    Light relaxation, drowsiness.
    ---
    Mid Values (50 to 200 uV^2):
    Deep sleep, unconscious processing.
    ---
    High Values (>200 uV^2):
    Very deep sleep, pathological state`"]
    P --> deltaLines
    thetaLines["`Lower Values (< 5 uV^2):
    HIgh Focus, external attention.
    ---
    Lower-mid Values (5 to 20 uV^2):
    Relaxed alertness, creativity, daydreaming.
    ---
    Mid Values (20 to 50 uV^2):
    Drowsiness, reduced attention, inward focus.
    ---
    High Values (>50 uV^2):
    Deep meditation, early sleep stages`"]
    Q --> thetaLines
```

### 🎨 **Audiovisual Effects Mapping**

```mermaid
graph TD;
    A[EEG_Alpha_Power] -->|Particle Density & Color Shifts| Effects;
    B[EEG_Beta_Power] -->|Particle Speed & Movement| Effects;
    C[HRV] -->|Ambient Drones & Scene Transitions| Effects;
    D[GSR_Spikes] -->|Short Bursts of Light & Sound Effects| Effects;
```

---

## 🎨 Audiovisual Mapping

| Biometric Data                  | Visual Effects               | Audio Effects           |
| ------------------------------- | ---------------------------- | ----------------------- |
| EEG Alpha Power (8–12Hz)        | Wave patterns, color shifts  | Reverb, drone harmonics |
| EEG Beta Power (12–30Hz)        | Rapid particle movement      | Percussion modulations  |
| HRV (Heart Rate Variability)    | Scene transitions            | Ambient pulses          |
| GSR Spikes (Emotional Arousal)  | Light bursts, glitch effects | Distorted textures      |
| Temperature (Emotional Valence) | Color temperature shifts     | Harmonic shifts         |

---

## 🏗️ Installation & Setup

🚀 Setting up your own RESI experience? Follow the guide in Installation_Environment_Setup.md for:

- ✅ Projector & speaker positioning
- ✅ EEG & biometric sensor placement
- ✅ Acoustic & lighting recommendations

---

## 🛠️ Development Workflow

👨‍💻 Want to contribute? Here’s the workflow:

Clone the repo:

```sh
git clone https://github.com/JoshPattani/ReSi_Rendered-Embodiment-of-Social-Interactions.git
cd ReSi_Rendered-Embodiment-of-Social-Interactions
```

Set up your Python environment:

```sh
python -m venv .venv

```

Activate the virtual environment:

```sh
source .venv/bin/activate  # On macOS/Linux
.venv\Scripts\activate     # On Windows
```

Install the required packages:

```sh
pip install -r requirements.txt
```

Run the data pipeline:

```sh
python main.py
```

Launch OpenBCI GUI and start streaming data.
Open Max/MSP, Unity, or Ableton Live and connect to OSC signals.

---

## 🤯 Future Enhancements

- 🔮 Complexity of EEG synchrony mapping
- 🔮 AI-driven emotional state prediction
- 🔮 Expanded sensor suite (ECG, EDA, motion tracking)
- 🔮 Haptic feedback integration

---

## 💡 Credits & Inspiration

### 🚀 Developed by: Cass Bliss, Josh Pattani, and Jazlin Rodriguez

### 🎨 Artistic Direction: Cass Bliss

### 🧠 Neurofeedback Design: Josh Pattani

### 🎥 Visuals & Interaction: Jazlin Rodriguez

---

## 📚 References & Resources

- 📖 [OpenBCI Cyton User Manual](https://docs.openbci.com/docs/02Cyton/CytonDataFormat)
- 📖 [LabstreamingLayer (LSL) Documentation](https://labstreaminglayer.readthedocs.io/)
- 📖 [Max/MSP Documentation](https://docs.cycling74.com/max8)

---

## 📅 Upcoming Events

- 🎤 Workshops and demos happening in 2025
- 📚 Publications and papers related to the project
- 🛠️ User feedback sessions on system enhancements

---

## 🌟 Community Contributions

- 🤝 Join our community discussions on project ideas and improvements.
- 💬 Share your experiences and feedback to enhance RESI.

---

🙌 Pull requests & collaborations welcome!
