#!/usr/bin/env python3
"""
Benchmark script for specio3 SPC file reader.
Tests performance on various file sizes and formats.
"""

import os
import sys
import time
import statistics
import psutil
from pathlib import Path

# Add the project root to Python path so we can import specio3
script_dir = Path(__file__).parent
project_root = script_dir.parent
sys.path.insert(0, str(project_root))

import specio3


def get_file_info(filepath):
    """Get file size and basic info."""
    stat = os.stat(filepath)
    return {
        'size_bytes': stat.st_size,
        'size_kb': stat.st_size / 1024,
        'size_mb': stat.st_size / (1024 * 1024)
    }


def benchmark_file(filepath, num_runs=5):
    """Benchmark reading a single SPC file."""
    file_info = get_file_info(filepath)
    
    # Warm up
    specio3.read_spc(filepath)
    
    times = []
    memory_usage = []
    
    for _ in range(num_runs):
        # Measure memory before
        process = psutil.Process()
        mem_before = process.memory_info().rss / (1024 * 1024)  # MB
        
        # Time the operation
        start_time = time.perf_counter()
        data = specio3.read_spc(filepath)
        end_time = time.perf_counter()
        
        # Measure memory after
        mem_after = process.memory_info().rss / (1024 * 1024)  # MB
        
        times.append(end_time - start_time)
        memory_usage.append(mem_after - mem_before)
        
        # Get spectrum info
        if len(data) > 0:
            x, y = data[0]
            num_spectra = len(data)
            num_points = len(x)
    
    return {
        'file_size_mb': file_info['size_mb'],
        'file_size_kb': file_info['size_kb'],
        'num_spectra': num_spectra,
        'num_points': num_points,
        'avg_time_ms': statistics.mean(times) * 1000,
        'min_time_ms': min(times) * 1000,
        'max_time_ms': max(times) * 1000,
        'std_time_ms': statistics.stdev(times) * 1000 if len(times) > 1 else 0,
        'avg_memory_mb': statistics.mean(memory_usage),
        'throughput_mb_per_sec': file_info['size_mb'] / statistics.mean(times)
    }


def main():
    """Run benchmarks on test data."""
    # Get the project root directory (parent of scripts)
    script_dir = Path(__file__).parent
    project_root = script_dir.parent
    test_data_dir = project_root / 'tests' / 'data'
    
    if not test_data_dir.exists():
        print(f"Test data directory not found: {test_data_dir}")
        return
    
    # Get all SPC files
    spc_files = list(test_data_dir.glob('*.spc'))
    
    if not spc_files:
        print("No SPC files found in test data directory")
        return
    
    print("specio3 Performance Benchmark")
    print("=" * 50)
    print(f"Testing {len(spc_files)} SPC files")
    print(f"Platform: {psutil.cpu_count()} CPU cores")
    print()
    
    results = []
    
    # Categorize files by size for better reporting
    small_files = []  # < 100 KB
    medium_files = []  # 100 KB - 200 KB
    large_files = []  # > 200 KB
    
    for filepath in sorted(spc_files):
        try:
            print(f"Benchmarking {filepath.name}...", end=' ')
            result = benchmark_file(str(filepath))
            result['filename'] = filepath.name
            results.append(result)
            
            # Categorize by size
            if result['file_size_kb'] < 100:
                small_files.append(result)
            elif result['file_size_kb'] < 200:
                medium_files.append(result)
            else:
                large_files.append(result)
            
            print(f"✓ {result['avg_time_ms']:.1f}ms")
            
        except Exception as e:
            print(f"✗ Error: {e}")
    
    print()
    print("Benchmark Results")
    print("=" * 50)
    
    # Overall statistics
    if results:
        avg_time = statistics.mean([r['avg_time_ms'] for r in results])
        avg_throughput = statistics.mean([r['throughput_mb_per_sec'] for r in results])
        avg_memory = statistics.mean([r['avg_memory_mb'] for r in results])
        
        print(f"Overall Performance:")
        print(f"  Average load time: {avg_time:.1f} ms")
        print(f"  Average throughput: {avg_throughput:.1f} MB/s")
        print(f"  Average memory usage: {avg_memory:.1f} MB")
        print()
    
    # Detailed results by category
    categories = [
        ("Small files (< 100 KB)", small_files),
        ("Medium files (100-200 KB)", medium_files),
        ("Large files (> 200 KB)", large_files)
    ]
    
    for category_name, category_files in categories:
        if not category_files:
            continue
            
        print(f"{category_name}:")
        print(f"{'File':<20} {'Size':<10} {'Points':<8} {'Time':<8} {'Throughput':<12} {'Memory':<8}")
        print("-" * 75)
        
        for result in category_files:
            print(f"{result['filename']:<20} "
                  f"{result['file_size_kb']:.0f} KB{'':<4} "
                  f"{result['num_points']:<8} "
                  f"{result['avg_time_ms']:.1f} ms{'':<2} "
                  f"{result['throughput_mb_per_sec']:.1f} MB/s{'':<4} "
                  f"{result['avg_memory_mb']:.1f} MB")
        
        if len(category_files) > 1:
            avg_time = statistics.mean([r['avg_time_ms'] for r in category_files])
            avg_throughput = statistics.mean([r['throughput_mb_per_sec'] for r in category_files])
            avg_memory = statistics.mean([r['avg_memory_mb'] for r in category_files])
            print("-" * 75)
            print(f"{'Average':<20} {'':<10} {'':<8} "
                  f"{avg_time:.1f} ms{'':<2} "
                  f"{avg_throughput:.1f} MB/s{'':<4} "
                  f"{avg_memory:.1f} MB")
        print()
    
    # Find best and worst performers
    if results:
        fastest = min(results, key=lambda x: x['avg_time_ms'])
        slowest = max(results, key=lambda x: x['avg_time_ms'])
        highest_throughput = max(results, key=lambda x: x['throughput_mb_per_sec'])
        
        print("Performance Highlights:")
        print(f"  Fastest file: {fastest['filename']} ({fastest['avg_time_ms']:.1f} ms)")
        print(f"  Slowest file: {slowest['filename']} ({slowest['avg_time_ms']:.1f} ms)")
        print(f"  Highest throughput: {highest_throughput['filename']} ({highest_throughput['throughput_mb_per_sec']:.1f} MB/s)")


if __name__ == '__main__':
    main()
