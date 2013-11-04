#!/bin/bash

totalErrors=0
for test in fts-*; do
    ./$test
    testErrors=$?
    if [ $testErrors -gt 0 ]; then
        $totalErrors=$((totalErrors+testErrors))
    fi
done

echo
echo "== Results =="
echo "$totalErrors test failed"

