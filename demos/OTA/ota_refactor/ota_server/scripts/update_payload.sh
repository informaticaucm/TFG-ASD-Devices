file1="../../../payload_project/build/payload.bin"
file2="../payload.bin"

if cmp -s "$file1" "$file2"; then
    echo 'The binary payload has not been updated'
else
    echo 'The binary payload has been updated'
    cp ../../../payload_project/build/payload.bin ../
    date +%s > ../binary_age.txt 
fi