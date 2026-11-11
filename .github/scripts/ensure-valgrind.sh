#!/usr/bin/env bash
# Memory checks must fail closed if their required tool cannot be installed.
set -euo pipefail

if command -v valgrind >/dev/null 2>&1; then
    valgrind --version
    exit 0
fi

apt_options=(-o Acquire::Retries=1 -o Acquire::http::Timeout=10
             -o Acquire::https::Timeout=10 -o APT::Update::Error-Mode=any)
for attempt in 1 2; do
    echo "Installing Valgrind (attempt ${attempt}/2)"
    if timeout 45s sudo apt-get "${apt_options[@]}" update &&
       timeout 75s sudo apt-get "${apt_options[@]}" install -y --no-install-recommends valgrind; then
        valgrind --version
        exit 0
    fi
    if [ "$attempt" -lt 2 ]; then
        sleep 3
    fi
done

echo "::error::Valgrind installation failed after bounded retries. Check runner DNS/package mirrors and rerun the job. Memory tests were not skipped."
exit 1
