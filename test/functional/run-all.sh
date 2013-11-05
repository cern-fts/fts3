#!/bin/bash

totalErrors=0
declare -a failed=()
for test in fts-*; do
    ./$test
    testErrors=$?
    if [ $testErrors -gt 0 ]; then
        $totalErrors=$((totalErrors+testErrors))
        failed=("${failed[@]}" "$test")
    fi
done

echo
echo "== Results =="
echo "$totalErrors test failed"
if [ $totalErrors -gt 0 ]; then
    echo ${failed[@]}
fi

