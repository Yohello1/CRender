# ./delete.sh
source /opt/intel/oneapi/setvars.sh
cmake -DCMAKE_BUILD_TYPE=RELEASE .
cmake --build . --target CRender
