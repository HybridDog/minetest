#! /bin/bash -e

# Use ccache if it is available
if command -v ccache >&-; then
	extra_args=(-DCMAKE_C_COMPILER_LAUNCHER=ccache \
		-DCMAKE_CXX_COMPILER_LAUNCHER=ccache)
fi

cmake -B build \
	"${extra_args[@]}" \
	-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE:-Debug} \
	-DRUN_IN_PLACE=TRUE \
	-DENABLE_GETTEXT=${CMAKE_ENABLE_GETTEXT:-TRUE} \
	-DBUILD_SERVER=${CMAKE_BUILD_SERVER:-TRUE} \
	${CMAKE_FLAGS}

cmake --build build --parallel $(($(nproc) + 1))
