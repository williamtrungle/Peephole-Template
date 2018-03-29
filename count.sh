#!/bin/bash

case "$1" in
    -v)
        verbose=1
        ;;
    -vv)
        verbose=2
        ;;
    *)
        verbose=0
        ;;
esac

echo -e "\033[93m"
echo "  Building Compiler"
echo "====================================="
echo -e -n "\033[0m"

if [ "$verbose" -gt 1 ]; then
    make -C JOOSA-src
    rm -f PeepholeBenchmarks/bench*/*.*dump
else
    make -C JOOSA-src 1>/dev/null
    rm -f PeepholeBenchmarks/bench*/*.*dump 1>/dev/null
fi

COUNT=0
COUNT_COMPILED=0

for BENCH_DIR in PeepholeBenchmarks/*/
do
	((COUNT++))

	BENCH=$(basename $BENCH_DIR)
    if [ "$verbose" -gt 0 ]; then
        echo -e "\033[93m"
        echo "  Generating Bytecode for '$BENCH'"
        echo "====================================="
        echo -e -n "\033[0m"
    fi

    if [ "$verbose" -gt 1 ]; then
        echo -e -n "\033[92m"
        echo "  Normal"
        echo "----------------"
        echo -e -n "\033[0m"
        PEEPDIR=`pwd` make -C $BENCH_DIR
    else
        PEEPDIR=`pwd` make -C $BENCH_DIR 1>/dev/null
    fi

    if [ "$verbose" -gt 0 ]; then
        if [ $? != 0 ]
        then
            echo
            echo -e "\e[41m\033[1mError: Unable to compile benchmark '$BENCH'\e[0m"
            continue
        fi
    fi

    if [ "$verbose" -gt 1 ]; then
        echo -e "\033[92m"
        echo "  Optimized"
        echo "----------------"
        echo -e -n "\033[0m"
        PEEPDIR=`pwd` make -C $BENCH_DIR opt
    else
        PEEPDIR=`pwd` make -C $BENCH_DIR opt 1>/dev/null
    fi

    if [ "$verbose" -gt 0 ]; then
        if [ $? != 0 ]
        then
            echo
            echo -e "\e[41m\033[1mError: Unable to optimize benchmark '$BENCH'\e[0m"
            continue
        fi

        if [ "$verbose" -gt 1 ]; then
            echo -e "\033[92m"
            echo "  Bytecode Size"
            echo "----------------"
            echo -e -n "\033[0m"
        fi
    fi

	NORMAL=$(grep -a code_length $BENCH_DIR*.dump | awk '{sum += $3} END {print sum}')
	OPT=$(grep -a code_length $BENCH_DIR*.optdump | awk '{sum += $3} END {print sum}')

    if [ "$verbose" -gt 0 ]; then
        if [[ -z "$NORMAL" || -z "$OPT" ]]
        then
            echo -e "\e[41m\033[1mERROR\e[0m"
            continue
        fi
    fi

	((COUNT_COMPILED++))

    if [ "$verbose" -gt 0 ]; then
        echo -e "\e[42m\033[1mNormal:\033[0m\e[42m $NORMAL\e[49m"
        echo -e "\e[42m\033[1mOptimized:\033[0m\e[42m $OPT\e[49m"
    fi
done

echo -e "\033[93m"
echo "  Overall Bytecode Size"
echo "====================================="
echo -e -n "\033[0m"

if [ $COUNT == $COUNT_COMPILED ]
then
	echo -e "\e[42mSuccessfully compiled all benchmarks\e[49m"
else
	echo -e "\e[41mError: Compiled $COUNT_COMPILED/$COUNT benchmarks\e[49m"
fi
echo

NORMAL=$(grep -a code_length PeepholeBenchmarks/bench*/*.dump | awk '{sum += $3} END {print sum}')
OPT=$(grep -a code_length PeepholeBenchmarks/bench*/*.optdump | awk '{sum += $3} END {print sum}')

if [[ -z "$NORMAL" || -z "$OPT" ]]
then
	echo -e "\e[41m\033[1mError: Unable to load bytecode statistics\e[0m"
	exit
fi

echo -e "\e[42m\033[1mNormal:\033[0m\e[42m $NORMAL\e[49m"
echo -e "\e[42m\033[1mOptimized:\033[0m\e[42m $OPT\e[49m"
