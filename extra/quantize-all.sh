#!/bin/bash

printf "Usage: $0 <upload>"

if [ $# -ne 1 ]; then
    printf "\nError: Invalid number of arguments\n"
    exit 1
fi

qtype0="q5_0"
qtype1="q5_1"
upload="$1"
declare -a filedex

cd `dirname $0`
cd ../

# Let's loop across all the objects in the 'models' dir:
for i in ./models/*; do
    # Check to see if it's a file or directory
    if [ -d "$i" ]; then
        # It's a directory! We should make sure it's not empty first:
        if [ "$(ls -A $i)" ]; then
            # Passed! Let's go searching for bin files (shouldn't need to go more than a layer deep here)
            for f in "$i"/*.bin; do
                # [Neuron Activation]
                newfile=`echo "${f##*/}" | cut -d _ -f 1`;
                if [ "$newfile" != "q5" ]; then
                    ./quantize "${f}" "${i:-4}/${i:9:${#i}-4}-${qtype1}.bin" ${qtype1};
                    ./quantize "${f}" "${i:-4}/${i:9:${#i}-4}-${qtype0}.bin" ${qtype0};
                    filedex+=( "${i:-4}/${i:9:${#i}-4}-${qtype1}.bin" "${i:-4}/${i:9:${#i}-4}-${qtype0}.bin" )
                fi
            done
        fi
    else
        # It's a file! Let's make sure it's the right type:
        if [ "${i##*.}" == "bin" ]; then
            # And we probably want to skip the testing files
            if [ "${i:9:8}" != "for-test" ]; then
                # [Neuron Activation]
                ./quantize "${i}" "${i:-4}-${qtype1}.bin" ${qtype1};
                ./quantize "${i}" "${i:-4}-${qtype0}.bin" ${qtype0};
                filedex+=( "${i:-4}-${qtype1}.bin" "${i:-4}-${qtype0}.bin" )
            fi
        fi
    fi
done



if [ "$upload" == "1" ]; then
    for i in ${!filedex[@]}; do
        if [ "${filedex[$i]:9:8}" != "for-test" ]; then
            scp ${filedex[$i]} root@linode0:/mnt/Data/ggml/ggml-model-${filedex[$i]:9}
        fi
    done
fi
