include(GenerateExportHeader)
include(CMakePackageConfigHelpers)
include(CtrlworkModule)

#======================================================
# Modules (one target each, see ctrlwork_add_module)
# If a module depends on another, add the dependency first.
#======================================================
 
add_subdirectory(bigint)        # CtrlWork::BigInt
add_subdirectory(bcd_packed)    # CtrlWork::BcdPacked
add_subdirectory(bcd_unpacked)  # CtrlWork::BcdUnpacked
add_subdirectory(matrix)        # CtrlWork::Matrix

#======================================================
# Umbrella target: CtrlWork::CtrlWork = all modules
#======================================================
 
get_property(_ctrlwork_modules GLOBAL PROPERTY CTRLWORK_MODULE_TARGETS)
 
add_library(CtrlWork INTERFACE)
add_library(${CTRLWORK_PACKAGE_NAME}::${CTRLWORK_PACKAGE_NAME} ALIAS CtrlWork)
set_target_properties(CtrlWork PROPERTIES EXPORT_NAME ${CTRLWORK_PACKAGE_NAME})
target_link_libraries(CtrlWork INTERFACE ${_ctrlwork_modules})
