#!/usr/bin/env bash
set -uo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(cd "${SCRIPT_DIR}/.." && pwd)"
BUILD_DIR="${PROJECT_DIR}/build"

RUN_CPPCHECK=1
RUN_CLANG_TIDY=1

CPPCHECK_FLAGS=(
    --enable=all
    --inconclusive
    --library=posix
    --platform=unix64
    --check-level=exhaustive
    --suppress=missingIncludeSystem
    --suppress=unusedFunction
    --error-exitcode=1
)

while [[ $# -gt 0 ]]; do
    case "$1" in
        --cppcheck-only)
            RUN_CPPCHECK=1
            RUN_CLANG_TIDY=0
            shift
            ;;
        --clang-tidy-only)
            RUN_CPPCHECK=0
            RUN_CLANG_TIDY=1
            shift
            ;;
        --build-dir)
            BUILD_DIR="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [--cppcheck-only | --clang-tidy-only] [--build-dir <dir>]"
            exit 0
            ;;
        *)
            echo "Unknown argument: $1"
            echo "Usage: $0 [--cppcheck-only | --clang-tidy-only] [--build-dir <dir>]"
            exit 1
            ;;
    esac
done

if [[ ! -f "${BUILD_DIR}/compile_commands.json" ]]; then
    echo "Error: ${BUILD_DIR}/compile_commands.json not found!"
    echo "Please configure the build directory first, e.g.:"
    echo "  cd ${PROJECT_DIR} && meson setup build"
    exit 1
fi

OVERALL_STATUS=0

# -------------------------------------------------------------
# 1. cppcheck
# -------------------------------------------------------------
if [[ "${RUN_CPPCHECK}" -eq 1 ]]; then
    echo "=========================================================="
    echo " Running cppcheck..."
    echo "=========================================================="
    if command -v cppcheck >/dev/null 2>&1; then
        if cppcheck --project="${BUILD_DIR}/compile_commands.json" \
            "${CPPCHECK_FLAGS[@]}"; then
            echo "[OK] cppcheck passed with no errors."
        else
            echo "[FAIL] cppcheck reported issues."
            OVERALL_STATUS=1
        fi
    else
        echo "[ERROR] cppcheck is not installed or not in PATH."
        OVERALL_STATUS=1
    fi
fi

# -------------------------------------------------------------
# 2. clang-tidy
# -------------------------------------------------------------
if [[ "${RUN_CLANG_TIDY}" -eq 1 ]]; then
    echo ""
    echo "=========================================================="
    echo " Running clang-tidy..."
    echo "=========================================================="
    if command -v clang-tidy >/dev/null 2>&1; then
        # Check if compile_commands.json contains gcc-specific -fanalyzer flag
        CDB_DIR="${BUILD_DIR}"
        CLEANUP_CDB_DIR=""

        if grep -q -- '-fanalyzer' "${BUILD_DIR}/compile_commands.json"; then
            TMP_CDB_DIR="$(mktemp -d -t cdb_tidy_XXXXXX)"
            sed 's/-fanalyzer//g' "${BUILD_DIR}/compile_commands.json" > "${TMP_CDB_DIR}/compile_commands.json"
            CDB_DIR="${TMP_CDB_DIR}"
            CLEANUP_CDB_DIR="${TMP_CDB_DIR}"
        fi

        mapfile -t SOURCES < <(find "${PROJECT_DIR}/src" -type f -name '*.cpp' | sort)

        if [[ ${#SOURCES[@]} -eq 0 ]]; then
            echo "No source files found in ${PROJECT_DIR}/src."
        else
            if clang-tidy -p "${CDB_DIR}" "${SOURCES[@]}"; then
                echo "[OK] clang-tidy passed with no errors."
            else
                echo "[FAIL] clang-tidy reported issues."
                OVERALL_STATUS=1
            fi
        fi

        if [[ -n "${CLEANUP_CDB_DIR}" && -d "${CLEANUP_CDB_DIR}" ]]; then
            rm -rf "${CLEANUP_CDB_DIR}"
        fi
    else
        echo "[ERROR] clang-tidy is not installed or not in PATH."
        OVERALL_STATUS=1
    fi
fi

echo ""
echo "=========================================================="
if [[ "${OVERALL_STATUS}" -eq 0 ]]; then
    echo " All linters completed successfully!"
else
    echo " Linters found issues (exit code: ${OVERALL_STATUS})."
fi
echo "=========================================================="

exit "${OVERALL_STATUS}"
