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
        description="Validate 1D FDTD reflection against Fresnel theory for a single dielectric interface."
    )

    parser.add_argument(
        "--reference",
        default="fdtd_reference.csv",
        help="Reference CSV generated without device."
    )

    parser.add_argument(
        "--interface",
        default="fdtd_interface.csv",
        help="Interface CSV generated with air-dielectric interface."
    )

    parser.add_argument(
        "--output",
        default="results/fresnel_validation.png",
        help="Output validation figure path."
    )

    parser.add_argument(
        "--n1",
        type=float,
        default=1.0,
        help="Refractive index of incident medium."
    )

    parser.add_argument(
        "--n2",
        type=float,
        default=1.5,
        help="Refractive index of transmitted medium."
    )

    parser.add_argument(
        "--dx-nm",
        type=float,
        default=10.0,
        help="Spatial grid step in nm. Currently assumed manually."
    )

    parser.add_argument(
        "--min-wavelength",
        type=float,
        default=500.0,
        help="Minimum wavelength shown in nm."
    )

    parser.add_argument(
        "--max-wavelength",
        type=float,
        default=1500.0,
        help="Maximum wavelength shown in nm."
    )

    args = parser.parse_args()

    reference_path = Path(args.reference)
    interface_path = Path(args.interface)
    output_path = Path(args.output)

    if not reference_path.exists():
        raise FileNotFoundError(
            f"Reference file not found: {reference_path}\n"
            "Please run: ./build_release/FDTD_1D_OOP_Solver reference"
        )

    if not interface_path.exists():
        raise FileNotFoundError(
            f"Interface file not found: {interface_path}\n"
            "Please run: ./build_release/FDTD_1D_OOP_Solver interface"
        )

    time_ref, ref_probe1, ref_probe2 = load_probe_csv(reference_path)
    time_int, int_probe1, int_probe2 = load_probe_csv(interface_path)

    n = min(len(time_ref), len(time_int))

    # Reference transmitted signal is used as the incident baseline.
    incident_reference = ref_probe2[:n]

    # Probe 1 in the interface simulation is in the scattered-field region,
    # so it mainly records the reflected wave.
    reflected_interface = int_probe1[:n]

    incident_reference = incident_reference - np.mean(incident_reference)
    reflected_interface = reflected_interface - np.mean(reflected_interface)

    inc_fft = np.fft.rfft(incident_reference)
    ref_fft = np.fft.rfft(reflected_interface)

    freq_bins = np.arange(len(inc_fft))

    # Remove DC component.
    inc_fft = inc_fft[1:]
    ref_fft = ref_fft[1:]
    freq_bins = freq_bins[1:]

    wavelengths_nm = (n * args.dx_nm) / freq_bins

    inc_power = np.abs(inc_fft) ** 2
    ref_power = np.abs(ref_fft) ** 2

    eps = 1e-30
    R_fdtd = ref_power / (inc_power + eps)

    R_fresnel = ((args.n1 - args.n2) / (args.n1 + args.n2)) ** 2

    valid = (
        (wavelengths_nm >= args.min_wavelength)
        & (wavelengths_nm <= args.max_wavelength)
        & (inc_power > np.max(inc_power) * 1e-6)
    )

    if np.count_nonzero(valid) < 10:
        valid = (
            (wavelengths_nm >= args.min_wavelength)
            & (wavelengths_nm <= args.max_wavelength)
        )

    R_band = R_fdtd[valid]
    wavelengths_band = wavelengths_nm[valid]

    R_median = np.median(R_band)
    R_mean = np.mean(R_band)
    abs_error = abs(R_median - R_fresnel)

    print("========== Fresnel validation ==========")
    print(f"n1:                  {args.n1}")
    print(f"n2:                  {args.n2}")
    print(f"Theoretical R:       {R_fresnel:.6f}")
    print(f"FDTD median R:       {R_median:.6f}")
    print(f"FDTD mean R:         {R_mean:.6f}")
    print(f"Absolute error:      {abs_error:.6f}")
    print(f"Samples used:        {n}")
    print(f"Valid spectrum pts:  {np.count_nonzero(valid)}")
    print("========================================")

    output_path.parent.mkdir(parents=True, exist_ok=True)

    plt.figure(figsize=(10, 5))

    plt.plot(
        wavelengths_band,
        R_band,
        label="FDTD reflectance",
        linewidth=1.5,
    )

    plt.axhline(
        R_fresnel,
        linestyle="--",
        linewidth=2,
        label=f"Fresnel theory R = {R_fresnel:.4f}",
    )

    plt.axhline(
        R_median,
        linestyle=":",
        linewidth=2,
        label=f"FDTD median R = {R_median:.4f}",
    )

    plt.xlabel("Wavelength (nm)")
    plt.ylabel("Reflectance")
    plt.title("Single-interface Fresnel Validation")
    plt.xlim(args.min_wavelength, args.max_wavelength)
    plt.ylim(0.0, max(0.15, np.percentile(R_band, 95) * 1.2))
    plt.grid(True, linestyle="--", alpha=0.5)
    plt.legend()
    plt.tight_layout()

    plt.savefig(output_path, dpi=300, bbox_inches="tight")

    print(f"Saved validation figure to: {output_path}")


if __name__ == "__main__":
    main()
