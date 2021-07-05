<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">

  <!-- Needed by any VcxProj -->
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.props" />
  <ImportGroup Label="ExtensionSettings">
  </ImportGroup>
  <ImportGroup Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'" Label="PropertySheets">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
  </ImportGroup>
  <ImportGroup Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'" Label="PropertySheets">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
  </ImportGroup>
  <ImportGroup Condition="'$(Configuration)|$(Platform)'=='Release|x64'" Label="PropertySheets">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
  </ImportGroup>
  <ImportGroup Condition="'$(Configuration)|$(Platform)'=='Debug|x64'" Label="PropertySheets">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
  </ImportGroup>
  <ImportGroup Condition="'$(Configuration)|$(Platform)'=='Release|Win32'" Label="PropertySheets">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
  </ImportGroup>
  <ImportGroup Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'" Label="PropertySheets">
    <Import Project="$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props" Condition="exists('$(UserRootDir)\Microsoft.Cpp.$(Platform).user.props')" />
  </ImportGroup>
  <PropertyGroup Label="UserMacros" />
  <!-- The WrappedTaskItems label is used by the conversion tool to identify the location where items
        associated with wrapped tasks will reside.-->
  <ItemGroup Label="WrappedTaskItems" />
  <ItemGroup>
    <!-- We only add items (e.g. form ClSourceFiles) that do not already exist (e.g in the ClCompile list), this avoids duplication -->
    <ClCompile Include="@(ClSourceFiles)" Exclude="@(ClCompile)" />
    <ResourceCompile Include="@(RcSourceFiles)" Exclude="@(ResourceCompile)" />
    <Midl Include="@(IdlSourceFiles)" Exclude="@(Midl)" />
    <MessageCompile Include="@(McSourceFiles)" Exclude="@(MessageCompile)" />
    <MASM Include="@(AsmSourceFiles)" Exclude="@(MASM)" />
    <GenerateBmf Include="@(MofSourceFiles)" Exclude="@(GenerateBmf)" />
  </ItemGroup>
  <!-- Set default environment variables, e.g. for stampinf -->
  <ItemGroup>
    <BuildMacro Include="SDK_INC_PATH">
      <Value>$(KIT_SHARED_INC_PATH)</Value>
      <EnvironmentVariable>true</EnvironmentVariable>
    </BuildMacro>
  </ItemGroup>
  <ItemGroup>
    <Inf Exclude="@(Inf)" Include="$(DDK_INF_FILES)" />
    <Inf Exclude="@(Inf)" Include="*.inf" />
    <FilesToPackage Include="$(TargetPath)" />
    <FilesToPackage Include="$(DDK_PACKAGE_FILES)" />
  </ItemGroup>
  <!-- Necessary to pick up propper files from local directory when in the IDE-->
  <ItemGroup>
    <None Exclude="@(None)" Include="*.txt;*.htm;*.html" />
    <None Exclude="@(None)" Include="*.ico;*.cur;*.bmp;*.dlg;*.rct;*.gif;*.jpg;*.jpeg;*.wav;*.jpe;*.tiff;*.tif;*.png;*.rc2" />
    <None Exclude="@(None)" Include="*.def;*.bat;*.hpj;*.asmx" />
  </ItemGroup>
  <ItemGroup>
    <ClInclude Exclude="@(ClInclude)" Include="*.h;*.hpp;*.hxx;*.hm;*.inl;*.xsd" />
  </ItemGroup>
  <ItemDefinitionGroup>
    <ClCompile>
      <ControlFlowGuard>false</ControlFlowGuard>
    </ClCompile>
  </ItemDefinitionGroup>
  <!-- /Necessary to pick up propper files from local directory when in the IDE-->
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.targets" />

</Project>

