#!/usr/bin/env python3
"""
Comparison benchmark between specio3 and spectrochempy.
"""

import sys
import time
import statistics
import warnings
from pathlib import Path

# Add the project root to Python path so we can import specio3
script_dir = Path(__file__).parent
project_root = script_dir.parent
sys.path.insert(0, str(project_root))

import specio3

# Suppress spectrochempy warnings for cleaner output
warnings.filterwarnings('ignore')

try:
    import spectrochempy as scp
    HAS_SPECTROCHEMPY = True
except ImportError:
    HAS_SPECTROCHEMPY = False


def benchmark_specio3(filepath, num_runs=5):
    """Benchmark specio3."""
    # Warm up
    specio3.read_spc(filepath)
    
    times = []
    for _ in range(num_runs):
        start_time = time.perf_counter()
        data = specio3.read_spc(filepath)
        end_time = time.perf_counter()
        times.append(end_time - start_time)
    
    return statistics.mean(times)


def benchmark_spectrochempy(filepath, num_runs=5):
    """Benchmark spectrochempy."""
    if not HAS_SPECTROCHEMPY:
        return None
    
    # Warm up
    scp.read(str(filepath))
    
    times = []
    for _ in range(num_runs):
        start_time = time.perf_counter()
        data = scp.read(str(filepath))
        end_time = time.perf_counter()
        times.append(end_time - start_time)
    
    return statistics.mean(times)


def main():
    """Run comparison benchmark."""
    # Get the project root directory (parent of scripts)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    test_data_dir = project_root / 'tests' / 'data'
    
    # Test on a few representative files
    test_files = [
        '103b4anh.spc',  # ~131 KB, 33185 points
        '087b4ana.spc',  # ~258 KB, 66112 points
        '040b4ana.spc',  # ~131 KB, 33185 points
    ]
    
    print("specio3 vs spectrochempy Performance Comparison")
    print("=" * 60)
    
    if not HAS_SPECTROCHEMPY:
        print("⚠️  spectrochempy not available - install with: pip install spectrochempy")
        return
    
    print(f"{'File':<15} {'Size':<8} {'specio3':<10} {'spectrochempy':<15} {'Speedup':<8}")
    print("-" * 60)
    
    total_speedup = []
    
    for filename in test_files:
        filepath = test_data_dir / filename
        if not filepath.exists():
            continue
        
        file_size_kb = filepath.stat().st_size / 1024
        
        # Benchmark both libraries
        specio3_time = benchmark_specio3(str(filepath))
        scp_time = benchmark_spectrochempy(str(filepath))
        
        if scp_time is not None:
            speedup = scp_time / specio3_time
            total_speedup.append(speedup)
            
            print(f"{filename:<15} "
                  f"{file_size_kb:.0f} KB{'':<3} "
                  f"{specio3_time*1000:.1f} ms{'':<4} "
                  f"{scp_time*1000:.1f} ms{'':<8} "
                  f"{speedup:.1f}x")
    
    if total_speedup:
        avg_speedup = statistics.mean(total_speedup)
        print("-" * 60)
        print(f"Average speedup: {avg_speedup:.1f}x faster than spectrochempy")


if __name__ == '__main__':
    main()
