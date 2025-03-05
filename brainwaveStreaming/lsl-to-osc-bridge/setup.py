from setuptools import setup, find_packages

setup(
    name="lsl-to-osc-bridge",
    version="0.1.0",
    author="Jockomo",
    author_email="jockomo.eth@gmail.com",
    description="A bridge to connect LSL (Lab Streaming Layer) to OSC (Open Sound Control) for real-time biometric data processing.",
    packages=find_packages(where="src"),
    package_dir={"": "src"},
    install_requires=["python-osc", "pylsl", "pyyaml", "numpy"],
    entry_points={
        "console_scripts": [
            "lsl-to-osc-bridge=bridge:main",  # Assuming you will have a main function in bridge.py
        ],
    },
    classifiers=[
        "Programming Language :: Python :: 3",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
    ],
    python_requires=">=3.6",
)
