#!/bin/bash

totalErrors=0
declare -a failed=()
for test in fts-*; do
    echo "Run $test"
    ./$test
    testErrors=$?
    if [ $testErrors -gt 0 ]; then
        totalErrors=$((totalErrors+testErrors))
        failed=("${failed[@]}" "$test")
    elif [ $testErrors -lt 0 ]; then
        echo "Test aborted. Canceling"
        exit -1
    fi
done

echo
echo "== Results =="
echo "$totalErrors test failed"
if [ $totalErrors -gt 0 ]; then
    echo ${failed[@]}
else
    echo "Everything looks all right!"
fi

exit $totalErrors
