import argparse
from pathlib import Path

import numpy as np
import matplotlib.pyplot as plt


def load_probe_csv(path):
    data = np.loadtxt(path, delimiter=",", skiprows=1)

    if data.ndim == 1:
        data = data.reshape(1, -1)

    time_step = data[:, 0]
    probe1 = data[:, 1]
    probe2 = data[:, 2]

    return time_step, probe1, probe2


def main():
    parser = argparse.ArgumentParser(
        description="Compute reflection/transmission spectra using reference and device FDTD simulations."
    )
    parser.add_argument(
        "--reference",
        default="fdtd_reference.csv",
        help="Reference CSV generated without device."
    )
    parser.add_argument(
        "--device",
        default="fdtd_device.csv",
        help="Device CSV generated with optical structure."
    )
    parser.add_argument(
        "--output",
        default="results/reflection_transmission_spectrum.png",
        help="Output figure path."
    )
    parser.add_argument(
        "--dx-nm",
        type=float,
        default=10.0,
        help="Spatial step in nm. Currently assumed manually."
    )
    parser.add_argument(
        "--min-wavelength",
        type=float,
        default=300.0,
        help="Minimum wavelength shown in nm."
    )
    parser.add_argument(
        "--max-wavelength",
        type=float,
        default=1600.0,
        help="Maximum wavelength shown in nm."
    )

    args = parser.parse_args()

    reference_path = Path(args.reference)
    device_path = Path(args.device)
    output_path = Path(args.output)

    if not reference_path.exists():
        raise FileNotFoundError(
            f"Reference file not found: {reference_path}\n"
            "Please run: ./build_release/FDTD_1D_OOP_Solver reference"
        )

    if not device_path.exists():
        raise FileNotFoundError(
            f"Device file not found: {device_path}\n"
            "Please run: ./build_release/FDTD_1D_OOP_Solver device"
        )

    time_ref, ref_probe1, ref_probe2 = load_probe_csv(reference_path)
    time_dev, dev_probe1, dev_probe2 = load_probe_csv(device_path)

    # Keep the same length in case auto-shutoff stops two runs at slightly different times.
    n = min(len(time_ref), len(time_dev))

    time_ref = time_ref[:n]
    ref_probe2 = ref_probe2[:n]

    time_dev = time_dev[:n]
    dev_reflected = dev_probe1[:n]
    dev_transmitted = dev_probe2[:n]

    # Reference transmitted probe is used as incident baseline.
    incident_reference = ref_probe2

    # Remove DC offsets.
    incident_reference = incident_reference - np.mean(incident_reference)
    dev_reflected = dev_reflected - np.mean(dev_reflected)
    dev_transmitted = dev_transmitted - np.mean(dev_transmitted)

    # Apply a window to reduce FFT leakage.
    window = np.hanning(n)

    inc_fft = np.fft.fft(incident_reference * window)
    ref_fft = np.fft.fft(dev_reflected * window)
    trn_fft = np.fft.fft(dev_transmitted * window)

    half_n = n // 2
    m_bins = np.arange(1, half_n)

    inc_fft = inc_fft[1:half_n]
    ref_fft = ref_fft[1:half_n]
    trn_fft = trn_fft[1:half_n]

    wavelengths_nm = (n * args.dx_nm) / m_bins

    inc_power = np.abs(inc_fft) ** 2
    ref_power = np.abs(ref_fft) ** 2
    trn_power = np.abs(trn_fft) ** 2

    eps = 1e-30

    R = ref_power / (inc_power + eps)
    T = trn_power / (inc_power + eps)

    valid = (
        (wavelengths_nm >= args.min_wavelength)
        & (wavelengths_nm <= args.max_wavelength)
        & (inc_power > np.max(inc_power) * 1e-8)
    )

    if np.count_nonzero(valid) < 10:
        valid = (
            (wavelengths_nm >= args.min_wavelength)
            & (wavelengths_nm <= args.max_wavelength)
        )

    print("========== Spectrum extraction info ==========")
    print(f"Reference file: {reference_path}")
    print(f"Device file:    {device_path}")
    print(f"Samples used:   {n}")
    print(f"dx:             {args.dx_nm} nm")
    print(f"Valid points:   {np.count_nonzero(valid)}")
    print("==============================================")

    output_path.parent.mkdir(parents=True, exist_ok=True)

    plt.figure(figsize=(10, 5))

    plt.plot(
        wavelengths_nm[valid],
        R[valid],
        label="Reflectance R",
        linewidth=2,
    )

    plt.plot(
        wavelengths_nm[valid],
        T[valid],
        label="Transmittance T",
        linewidth=2,
    )

    plt.plot(
        wavelengths_nm[valid],
        (R + T)[valid],
        label="R + T",
        linestyle="--",
        linewidth=2,
    )

    plt.xlabel("Wavelength (nm)")
    plt.ylabel("Power ratio")
    plt.title("Reflection and Transmission Spectrum")
    plt.xlim(args.min_wavelength, args.max_wavelength)
    plt.ylim(-0.05, 1.2)
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()

    plt.savefig(output_path, dpi=300, bbox_inches="tight")

    print(f"Saved figure to: {output_path}")


if __name__ == "__main__":
    main()
