#! /bin/bash -eu

# Use ccache if it is available
if command -v ccache >&-; then
	extra_args=(-DCMAKE_C_COMPILER_LAUNCHER=ccache \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi

cd /tmp
git clone --recursive https://github.com/jupp0r/prometheus-cpp
mkdir prometheus-cpp/build
cd prometheus-cpp/build
cmake .. \
	"${extra_args[@]}" \
	-DCMAKE_INSTALL_PREFIX=/usr/local \
	-DCMAKE_BUILD_TYPE=Release \
	-DENABLE_TESTING=0
make -j$(nproc)
sudo make install

