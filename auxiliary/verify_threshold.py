#!/usr/bin/env python3
"""
Mathematical verification that threshold = 50.0 gives FPR ≈ 1 in 100,000

This script computes P(χ² > 50.0) for chi-squared distribution with df=15
using the incomplete gamma function, which is the mathematically correct
way to evaluate the chi-squared CDF.
"""

import math


def chi2_cdf_upper_tail(x, df):
    """
    Compute P(χ² > x) for chi-squared distribution with df degrees of freedom.

    Uses series expansion for the regularized lower incomplete gamma function
    to compute P(χ² ≤ x), then returns 1 - P(χ² ≤ x).

    The chi-squared distribution with df degrees of freedom is related to
    the gamma distribution: χ²(df) ~ Gamma(df/2, scale=2)

    P(χ² ≤ x) = γ(df/2, x/2) / Γ(df/2)

    where γ is the lower incomplete gamma function.

    Args:
        x: Value at which to evaluate the upper tail probability
        df: Degrees of freedom

    Returns:
        P(χ² > x)
    """
    k = df / 2.0
    t = x / 2.0

    if x <= 0:
        return 1.0

    # Compute regularized lower incomplete gamma using series expansion
    # P(χ² ≤ x) = [t^k * e^(-t) / Γ(k)] * Σ[n=0,∞] t^n / Γ(k+n+1)

    # Use log to avoid overflow
    log_term = k * math.log(t) - t - math.lgamma(k + 1)

    # Series expansion
    sum_val = 1.0
    term = 1.0
    n = 1

    while abs(term) > 1e-15 and n < 2000:
        term *= t / (k + n)
        sum_val += term
        n += 1

    # P(χ² ≤ x)
    cdf = math.exp(log_term) * sum_val

    # P(χ² > x) = 1 - P(χ² ≤ x)
    return 1.0 - cdf


def verify_threshold():
    """
    Verify that threshold = 50.0 with df=15 gives FPR ≈ 1 in 100,000
    """
    threshold = 50.0
    df = 15

    print("=" * 70)
    print("Mathematical Verification of Threshold = 50.0")
    print("=" * 70)
    print()
    print("Chi-Squared Goodness-of-Fit Test")
    print("-" * 70)
    print()
    print(f"Test statistic: χ² = Σ(Oᵢ - E)² / E")
    print(f"Degrees of freedom: df = {df} (16 bins - 1)")
    print(f"Null hypothesis: Data comes from uniform distribution")
    print()
    print("Under H₀, the test statistic follows χ²(15) distribution")
    print()

    # Compute the FPR
    fpr = chi2_cdf_upper_tail(threshold, df)

    print("=" * 70)
    print("Result")
    print("=" * 70)
    print()
    print(f"Threshold: χ² = {threshold}")
    print(f"P(χ² > {threshold} | df={df}) = {fpr:.10f}")
    print()
    print(f"False Positive Rate:")
    print(f"  Scientific notation: {fpr:.6e}")
    print(f"  As fraction: 1 in {1/fpr:.0f}")
    print(f"  As percentage: {fpr*100:.5f}%")
    print()

    # Verify it's close to 1e-5
    target = 1e-5
    error = abs(fpr - target)
    relative_error = error / target * 100

    print(f"Target FPR: {target:.6e} (1 in {1/target:.0f})")
    print(f"Absolute error: {error:.6e}")
    print(f"Relative error: {relative_error:.2f}%")
    print()

    if relative_error < 1:
        print("✓ Threshold verified: FPR matches target within 1%")
    else:
        print(f"⚠ Warning: Relative error is {relative_error:.2f}%")
    print()

    # Show entropy loss
    entropy_loss = -math.log2(1 - fpr)
    print("=" * 70)
    print("Entropy Loss Analysis")
    print("=" * 70)
    print()
    print(f"Entropy loss per test: {entropy_loss:.6e} bits")
    print(f"For 128-bit string: {entropy_loss:.10f} bits ({entropy_loss/128*100:.7f}%)")
    print()
    print("Conclusion: Entropy loss is completely negligible")
    print()

    # Show comparison with other thresholds
    print("=" * 70)
    print("Common Threshold Values")
    print("=" * 70)
    print()
    print(f"{'Threshold':<12} {'FPR':<15} {'Rate':<20} {'Use Case':<25}")
    print("-" * 70)

    thresholds = [
        (37.70, "1 in 1,000", "Aggressive testing"),
        (44.26, "1 in 10,000", "Balanced"),
        (50.00, "1 in 100,000", "Conservative (current)"),
        (56.49, "1 in 1,000,000", "Very conservative"),
    ]

    for t, rate_str, use_case in thresholds:
        fpr_t = chi2_cdf_upper_tail(t, df)
        print(f"{t:<12.2f} {fpr_t:<15.6e} {rate_str:<20} {use_case:<25}")

    print()
    print("=" * 70)
    print("Mathematical Foundation")
    print("=" * 70)
    print()
    print("The chi-squared CDF is computed using the incomplete gamma function:")
    print()
    print("  P(χ² ≤ x) = γ(df/2, x/2) / Γ(df/2)")
    print()
    print("Where:")
    print("  - γ(k, x) = ∫[0,x] t^(k-1) * e^(-t) dt  (lower incomplete gamma)")
    print("  - Γ(k) = ∫[0,∞] t^(k-1) * e^(-t) dt     (complete gamma)")
    print()
    print("The false positive rate (FPR) is:")
    print()
    print("  FPR = P(χ² > threshold) = 1 - P(χ² ≤ threshold)")
    print()
    print("This probability is constant regardless of sample size, which is")
    print("a fundamental property of the chi-squared goodness-of-fit test.")
    print()


if __name__ == "__main__":
    verify_threshold()
