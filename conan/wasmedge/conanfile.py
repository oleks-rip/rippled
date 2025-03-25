from conans import ConanFile, tools
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, copy
import os
import json

required_conan_version = ">=1.55.0"

class WasmedgeConan(ConanFile):
    name = "wasmedge"
    version = "0.14.1"
    license = "Apache License v2.0"
    url = "https://github.com/WasmEdge/WasmEdge.git"
    description = "Lightweight, high-performance, and extensible WebAssembly runtime"
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain", "CMakeDeps"
    requires = [("llvm/20.1.1@")]

    def export_sources(self):
        export_conandata_patches(self)
        pass


    #def build_requirements(self):
    #    self.tool_requires("llvm/20.1.1")


    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC


    def layout(self):
        cmake_layout(self, src_folder="src")


    def source(self):

        git = tools.Git()
        #git.clone("https://github.com/bytecodealliance/wasm-micro-runtime.git", "913c2227bad6cbcf06835b0648aca24ac235ff76",shallow=True)
        get(self, **self.conan_data["sources"][self.version], strip_root=True)


    def generate(self):
        tc = CMakeToolchain(self)

        tc.variables["WASMEDGE_USE_LLVM"] = 0
        tc.variables["WASMEDGE_BUILD_SHARED_LIB"] = 0
        tc.variables["WASMEDGE_BUILD_AOT_RUNTIME"] = 0
        tc.variables["WASMEDGE_BUILD_STATIC_LIB"] = 1
        tc.variables["WASMEDGE_BUILD_TOOLS"] = 0
        tc.variables["WASMEDGE_LINK_LLVM_STATIC"] = 1
        tc.variables["FMT_INSTALL"] = 1
        tc.variables["WASMEDGE_CFLAGS"] = "-Wno-deprecated-declarations"

        #ll_dep = self.dependencies["llvm"]
        #tc.variables["WASMEDGE_USE_LLVM"] = 1
        #self.output.info(f"-----------package_folder: {type(ll_dep.__dict__)}")
        #tc.variables["LLVM_DIR"] = os.path.join(ll_dep.package_folder, "lib", "cmake", "llvm")

        tc.generate()

        # This generates "foo-config.cmake" and "bar-config.cmake" in self.generators_folder
        deps = CMakeDeps(self)
        deps.generate()


    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.verbose = True
        cmake.configure()
        cmake.build()
        #self.run(f'echo {self.source_folder}')
        # Explicit way:
        # self.run('cmake %s/hello %s' % (self.source_folder, cmake.command_line))
        # self.run("cmake --build . %s" % cmake.build_config)


    def package(self):
        cmake = CMake(self)
        cmake.verbose = True
        cmake.install()


    def package_info(self):
        self.cpp_info.libs = ["wasmedge", "fmt"]
        self.cpp_info.names["cmake_find_package"] = "wasmedge"
        self.cpp_info.names["cmake_find_package_multi"] = "wasmedge"

