			       	NVIDIA Corporation

===============================================================================
                              DevX SDK integration README
                           Rev 2.90, April, 2022
===============================================================================
1. Find the MLNX_WinOF2_DevX_SDK_<version>.exe file in the WinOF-2 package.
2. Install the SDK in the development system.
   The SDK installation will expose the below environment variables required by components using the SDK:
   - variable name: DEVX_LIB_PATH - the path to the DevX lib file.
   - variable name: DEVX_INC_PATH - the path to the DevX header files.
3. Make sure the session used for compilation is restarted after installation to allow detection of the environment variables.
4. In case the library is installed, and the environment variables were not created after installation, 
   It is required to export the environment variables manually to point to the SDK lib and header files.
