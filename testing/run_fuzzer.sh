#!/bin/bash
# AFL++ Fuzzing Script
# CSE 731 Software Testing Project

set -e

echo "================================"
echo "AFL++ Fuzzing Execution"
echo "================================"
echo ""

# Check binary
if [ ! -f "build/config_parser_fuzz" ]; then
    echo "ERROR: Fuzzing binary not found!"
    echo "Run: make fuzz"
    exit 1
fi

# Check corpus
if [ ! -d "tests/corpus" ] || [ -z "$(ls -A tests/corpus)" ]; then
    echo "ERROR: No corpus files found!"
    exit 1
fi

# Duration (default: 1 hour)
DURATION=${1:-3600}

echo "Configuration:"
echo "  Binary: build/config_parser_fuzz"
echo "  Corpus: tests/corpus ($(ls -1 tests/corpus/*.txt 2>/dev/null | wc -l) files)"
echo "  Duration: ${DURATION} seconds"
echo ""

# Create output directory
mkdir -p tests/output

# Set environment variables for macOS
export AFL_SKIP_CPUFREQ=1
export AFL_I_DONT_CARE_ABOUT_MISSING_CRASHES=1
export AFL_NO_FORKSRV=1
export AFL_NO_AFFINITY=1
export ASAN_OPTIONS=detect_leaks=0:symbolize=0:abort_on_error=1

echo "Starting AFL++ (no fork server for macOS)..."
echo ""

# Run AFL++ without fork server
AFLplusplus/afl-fuzz \
    -i tests/corpus \
    -o tests/output \
    -V ${DURATION} \
    -m none \
    -t 5000 \
    -- ./build/config_parser_fuzz @@ 2>&1 | tee fuzzing_log.txt

echo ""
echo "================================"
echo "Fuzzing Complete!"
echo "================================"
echo ""

# Show results
if [ -d "tests/output/default" ]; then
    CRASHES=$(ls -1 tests/output/default/crashes/id:* 2>/dev/null | wc -l)
    QUEUE=$(ls -1 tests/output/default/queue/id:* 2>/dev/null | wc -l)
    
    echo "Results:"
    echo "  Test cases: ${QUEUE}"
    echo "  Crashes: ${CRASHES}"
    echo ""
    
    if [ $CRASHES -gt 0 ]; then
        echo "🐛 Crashes found!"
        echo ""
        echo "Next steps:"
        echo "  make triage"
        echo "  make reproduce CRASH=tests/output/default/crashes/id:000000"
    else
        echo "✅ No crashes found"
    fi
    echo ""
    echo "View statistics: make stats"
fi

echo ""
