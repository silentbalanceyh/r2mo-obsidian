# MiMo-V2.5-Pro Near-200K Context Benchmark Report

## Test Configuration
- Model: MiMo-V2.5-Pro FP8
- Context Length: ~200K tokens (199K input + 32 output)
- GPU: 8x NVIDIA (96GB each)
- Sweep Parameters:
  - batched_tokens: 8192, 16384, 32768, 65536
  - max_seqs: 8, 12, 16, 24, 32

## Summary
- Tested configs: 20
- Valid configs: 15
- Failed configs: 5 (all 65536 series - OOM)

## Top 10 Results (by Total Throughput)

| Rank | Batched Tokens | Max Seqs | Total (tok/s) | Prompt (tok/s) | Completion (tok/s) | P50 (s) | P95 (s) | GPU0 Mem Before/After (MiB) |
|------|---------------|----------|---------------|----------------|-------------------|---------|---------|----------------------------|
| 1 | 32768 | 24 | 27728.7 | 27724.2 | 4.5 | 110.58 | 170.54 | 265840 / 273260 |
| 2 | 32768 | 32 | 27638.5 | 27634.1 | 4.4 | 141.42 | 228.85 | 265856 / 273276 |
| 3 | 32768 | 16 | 27267.2 | 27262.8 | 4.3 | 82.35 | 115.25 | 265840 / 273260 |
| 4 | 16384 | 32 | 27126.8 | 27122.5 | 4.3 | 134.05 | 228.94 | 268686 / 272298 |
| 5 | 16384 | 24 | 26874.0 | 26869.7 | 4.3 | 104.62 | 171.99 | 268688 / 272268 |
| 6 | 16384 | 16 | 26367.8 | 26363.5 | 4.2 | 76.02 | 119.13 | 268560 / 272240 |
| 7 | 32768 | 12 | 26049.4 | 26045.2 | 4.2 | 68.69 | 90.05 | 265830 / 273250 |
| 8 | 32768 | 8 | 25952.7 | 25948.5 | 4.2 | 55.84 | 59.70 | 265828 / 273248 |
| 9 | 8192 | 12 | 25179.1 | 25175.1 | 4.0 | 57.52 | 92.29 | 271610 / 272780 |
| 10 | 16384 | 12 | 25131.5 | 25127.5 | 4.0 | 61.26 | 93.37 | 268552 / 272230 |

## Key Findings

### Best Configuration
- **batched_tokens=32768, max_seqs=24**
- Total throughput: **27728.7 tok/s**
- GPU memory: ~273.3 GiB per card (after test)

### Recommendations
1. Use **batched_tokens=32768** for optimal throughput
2. **max_seqs=24** provides the best balance between throughput and latency
3. **65536 batched_tokens** requires more than 96GB VRAM per GPU (OOM)

### Observations
- 32768 series provides ~10% higher throughput than 16384 series
- Higher max_seqs (32) increases latency significantly (P50 > 130s)
- All configurations utilize near-maximum GPU memory (~272-273 GiB)
