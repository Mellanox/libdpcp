							Mellanox Technologies

===============================================================================
					      DevX Interface README
                           Rev 2.80, November, 2021
===============================================================================
1. How to Integrate Windows DevX in Your Development Environmet
===============================================================================   
1. Find the MLNX_WinOF2_DevX_SDK_<version>.exe file in the WinOF-2 Priver package.
2. Install the SDK in your development system.
   The SDK will expose the following new environment variables required for the library detection
   - variable name: DEVX_LIB_PATH will be the path to the DevX lib file.
   - variable name: DEVX_INC_PATH will be the path to the DevX header files.
3. Make sure you update your session before start compiling.
4. In case the library is installed, and the environment variable does not exist, 
   you may export the environment variable manually to point to the SDK path.