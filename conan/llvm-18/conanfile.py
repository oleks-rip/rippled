from conans import ConanFile
from conan.tools.cmake import CMake, CMakeToolchain, CMakeDeps, cmake_layout
from conan.tools.files import apply_conandata_patches, export_conandata_patches, get, copy
import os

required_conan_version = ">=1.53.0"

class Llvm_18Conan(ConanFile):
    name = "llvm-18"
    version = "18.1.8"
    license = "Apache License v2.0 with LLVM Exceptions"
    url = "https://github.com/llvm/llvm-project.git"
    description = "LLVM with fixes"
    package_type = "library"
    settings = "os", "compiler", "build_type", "arch"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = {"shared": False, "fPIC": True}
    generators = "CMakeToolchain", "CMakeDeps"
    #exports_sources = "patches/no_verify_fix_endpoints.patch"


    def export_sources(self):
        export_conandata_patches(self)


    def config_options(self):
        if self.settings.os == "Windows":
            del self.options.fPIC


    def layout(self):
        cmake_layout(self, src_folder="src")


    def source(self):
        get(self, **self.conan_data["sources"][self.version], strip_root=True)


    def generate(self):
        tc = CMakeToolchain(self)

        #tc.settings.build_type = "Release"
        tc.variables["LLVM_ENABLE_PROJECTS"] = "llvm"
        #tc.variables["LLVM_BUILD_LLVM_DYLIB"] = 1
        tc.generate()

        # This generates "foo-config.cmake" and "bar-config.cmake" in self.generators_folder
        #deps = CMakeDeps(self)
        #deps.generate()


    def build(self):
        apply_conandata_patches(self)
        cmake = CMake(self)
        cmake.verbose = True
        cmake.configure(build_script_folder=os.path.join(self.source_folder, "llvm"))
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
        self.cpp_info.libs = [
            "LLVMDemangle",
            "LLVMSupport",
            "LLVMTableGen",
            "LLVMTableGenGlobalISel",
            "LLVMTableGenCommon",
            "LLVMCore",
            "LLVMFuzzerCLI",
            "LLVMFuzzMutate",
            "LLVMFileCheck",
            "LLVMInterfaceStub",
            "LLVMIRPrinter",
            "LLVMIRReader",
            "LLVMCodeGenTypes",
            "LLVMCodeGen",
            "LLVMSelectionDAG",
            "LLVMAsmPrinter",
            "LLVMMIRParser",
            "LLVMGlobalISel",
            "LLVMBinaryFormat",
            "LLVMBitReader",
            "LLVMBitWriter",
            "LLVMBitstreamReader",
            "LLVMDWARFLinker",
            "LLVMDWARFLinkerClassic",
            "LLVMDWARFLinkerParallel",
            "LLVMExtensions",
            "LLVMFrontendDriver",
            "LLVMFrontendHLSL",
            "LLVMFrontendOpenACC",
            "LLVMFrontendOpenMP",
            "LLVMFrontendOffloading",
            "LLVMTransformUtils",
            "LLVMInstrumentation",
            "LLVMAggressiveInstCombine",
            "LLVMInstCombine",
            "LLVMScalarOpts",
            "LLVMipo",
            "LLVMVectorize",
            "LLVMObjCARCOpts",
            "LLVMCoroutines",
            "LLVMCFGuard",
            "LLVMHipStdPar",
            "LLVMLinker",
            "LLVMAnalysis",
            "LLVMLTO",
            "LLVMMC",
            "LLVMMCParser",
            "LLVMMCDisassembler",
            "LLVMMCA",
            "LLVMObjCopy",
            "LLVMObject",
            "LLVMObjectYAML",
            "LLVMOption",
            "LLVMRemarks",
            "LLVMDebuginfod",
            "LLVMDebugInfoDWARF",
            "LLVMDebugInfoGSYM",
            "LLVMDebugInfoLogicalView",
            "LLVMDebugInfoMSF",
            "LLVMDebugInfoCodeView",
            "LLVMDebugInfoPDB",
            "LLVMSymbolize",
            "LLVMDebugInfoBTF",
            "LLVMDWP",
            "LLVMExecutionEngine",
            "LLVMInterpreter",
            "LLVMJITLink",
            "LLVMMCJIT",
            "LLVMOrcJIT",
            "LLVMOrcDebugging",
            "LLVMOrcShared",
            "LLVMOrcTargetProcess",
            "LLVMRuntimeDyld",
            "LLVMTarget",
            "LLVMAArch64CodeGen",
            "LLVMAArch64AsmParser",
            "LLVMAArch64Disassembler",
            "LLVMAArch64Desc",
            "LLVMAArch64Info",
            "LLVMAArch64Utils",
            "LLVMAMDGPUCodeGen",
            "LLVMAMDGPUAsmParser",
            "LLVMAMDGPUDisassembler",
            "LLVMAMDGPUTargetMCA",
            "LLVMAMDGPUDesc",
            "LLVMAMDGPUInfo",
            "LLVMAMDGPUUtils",
            "LLVMARMCodeGen",
            "LLVMARMAsmParser",
            "LLVMARMDisassembler",
            "LLVMARMDesc",
            "LLVMARMInfo",
            "LLVMARMUtils",
            "LLVMAVRCodeGen",
            "LLVMAVRAsmParser",
            "LLVMAVRDisassembler",
            "LLVMAVRDesc",
            "LLVMAVRInfo",
            "LLVMBPFCodeGen",
            "LLVMBPFAsmParser",
            "LLVMBPFDisassembler",
            "LLVMBPFDesc",
            "LLVMBPFInfo",
            "LLVMHexagonCodeGen",
            "LLVMHexagonAsmParser",
            "LLVMHexagonDisassembler",
            "LLVMHexagonDesc",
            "LLVMHexagonInfo",
            "LLVMLanaiCodeGen",
            "LLVMLanaiAsmParser",
            "LLVMLanaiDisassembler",
            "LLVMLanaiDesc",
            "LLVMLanaiInfo",
            "LLVMLoongArchCodeGen",
            "LLVMLoongArchAsmParser",
            "LLVMLoongArchDisassembler",
            "LLVMLoongArchDesc",
            "LLVMLoongArchInfo",
            "LLVMMipsCodeGen",
            "LLVMMipsAsmParser",
            "LLVMMipsDisassembler",
            "LLVMMipsDesc",
            "LLVMMipsInfo",
            "LLVMMSP430CodeGen",
            "LLVMMSP430Desc",
            "LLVMMSP430Info",
            "LLVMMSP430AsmParser",
            "LLVMMSP430Disassembler",
            "LLVMNVPTXCodeGen",
            "LLVMNVPTXDesc",
            "LLVMNVPTXInfo",
            "LLVMPowerPCCodeGen",
            "LLVMPowerPCAsmParser",
            "LLVMPowerPCDisassembler",
            "LLVMPowerPCDesc",
            "LLVMPowerPCInfo",
            "LLVMRISCVCodeGen",
            "LLVMRISCVAsmParser",
            "LLVMRISCVDisassembler",
            "LLVMRISCVDesc",
            "LLVMRISCVTargetMCA",
            "LLVMRISCVInfo",
            "LLVMSparcCodeGen",
            "LLVMSparcAsmParser",
            "LLVMSparcDisassembler",
            "LLVMSparcDesc",
            "LLVMSparcInfo",
            "LLVMSystemZCodeGen",
            "LLVMSystemZAsmParser",
            "LLVMSystemZDisassembler",
            "LLVMSystemZDesc",
            "LLVMSystemZInfo",
            "LLVMVECodeGen",
            "LLVMVEAsmParser",
            "LLVMVEDisassembler",
            "LLVMVEInfo",
            "LLVMVEDesc",
            "LLVMWebAssemblyCodeGen",
            "LLVMWebAssemblyAsmParser",
            "LLVMWebAssemblyDisassembler",
            "LLVMWebAssemblyDesc",
            "LLVMWebAssemblyInfo",
            "LLVMWebAssemblyUtils",
            "LLVMX86CodeGen",
            "LLVMX86AsmParser",
            "LLVMX86Disassembler",
            "LLVMX86TargetMCA",
            "LLVMX86Desc",
            "LLVMX86Info",
            "LLVMXCoreCodeGen",
            "LLVMXCoreDisassembler",
            "LLVMXCoreDesc",
            "LLVMXCoreInfo",
            "LLVMAsmParser",
            "LLVMLineEditor",
            "LLVMProfileData",
            "LLVMCoverage",
            "LLVMPasses",
            "LLVMTargetParser",
            "LLVMTextAPI",
            "LLVMTextAPIBinaryReader",
            "LLVMDlltoolDriver",
            "LLVMLibDriver",
            "LLVMXRay",
            "LLVMWindowsDriver",
            "LLVMWindowsManifest",
            "LLVMCFIVerify",
            "LLVMDiff",
            "LLVMExegesisX86",
            "LLVMExegesisAArch64",
            "LLVMExegesisPowerPC",
            "LLVMExegesisMips",
            "LLVMExegesis",
        ]


