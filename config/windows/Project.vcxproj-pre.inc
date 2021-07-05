<?xml version="1.0" encoding="utf-8"?>
<Project DefaultTargets="Build" ToolsVersion="4.0" xmlns="http://schemas.microsoft.com/developer/msbuild/2003">

  <ItemGroup Label="ProjectConfigurations">
    <ProjectConfiguration Include="Debug|ARM64" >
      <Configuration>Debug</Configuration>
      <Platform>ARM64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|ARM64" >
      <Configuration>Release</Configuration>
      <Platform>ARM64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Debug|x64" >
      <Configuration>Debug</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|x64" >
      <Configuration>Release</Configuration>
      <Platform>x64</Platform>
    </ProjectConfiguration>
	<ProjectConfiguration Include="Debug|Win32" >
      <Configuration>Debug</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
    <ProjectConfiguration Include="Release|Win32" >
      <Configuration>Release</Configuration>
      <Platform>Win32</Platform>
    </ProjectConfiguration>
  </ItemGroup>
  <PropertyGroup Label="Globals">
    <VCTargetsPath Condition="'$(VCTargetsPath11)' != '' and '$(VisualStudioVersion)' == '11.0'">$(VCTargetsPath11)</VCTargetsPath>
  </PropertyGroup>
  <Import Project="$(VCTargetsPath)\Microsoft.Cpp.Default.props" />
  <ImportGroup Label="PreConfiguration">
    <Import Project="$(VCTargetsPath)\WDKConversion\PreConfiguration.props" />
  </ImportGroup>
  <PropertyGroup Label="PropertySheets">
    <DebuggerFlavor Condition="'$(IsKernelModeToolset)'=='true'">DbgengKernelDebugger</DebuggerFlavor>
    <DebuggerFlavor Condition="'$(IsUserModeToolset)'=='true'">DbgengRemoteDebugger</DebuggerFlavor>
  </PropertyGroup>
  <Choose>
    <When Condition="Exists('$(MSBuildProgramFiles32)\Windows Kits\10')">
        <PropertyGroup >
            <UMToolset>WindowsApplicationForDrivers10.0</UMToolset>
            <UMTargetVersion>Windows10</UMTargetVersion>
        </PropertyGroup>
    </When>
    <Otherwise >
        <PropertyGroup>
            <UMToolset>WindowsApplicationForDrivers8.1</UMToolset>
            <UMTargetVersion>WindowsV6.3</UMTargetVersion>
        </PropertyGroup>
    </Otherwise>
 </Choose>
  <PropertyGroup Label="Configuration" Condition="'$(Configuration)|$(Platform)'=='Debug|ARM64'">
    <TargetVersion>$(UMTargetVersion)</TargetVersion>
	<UseDebugLibraries>True</UseDebugLibraries>
    <PlatformToolset>$(UMToolset)</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Label="Configuration" Condition="'$(Configuration)|$(Platform)'=='Release|ARM64'">
    <TargetVersion>$(UMTargetVersion)</TargetVersion>
    <UseDebugLibraries>False</UseDebugLibraries>
    <PlatformToolset>$(UMToolset)</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Label="Configuration" Condition="'$(Configuration)|$(Platform)'=='Debug|x64'">
    <TargetVersion>$(UMTargetVersion)</TargetVersion>
	<UseDebugLibraries>True</UseDebugLibraries>
    <PlatformToolset>$(UMToolset)</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Label="Configuration" Condition="'$(Configuration)|$(Platform)'=='Release|x64'">
    <TargetVersion>$(UMTargetVersion)</TargetVersion>
	<UseDebugLibraries>False</UseDebugLibraries>
    <PlatformToolset>$(UMToolset)</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Label="Configuration" Condition="'$(Configuration)|$(Platform)'=='Debug|Win32'">
    <TargetVersion>$(UMTargetVersion)</TargetVersion>
	<UseDebugLibraries>True</UseDebugLibraries>
    <PlatformToolset>$(UMToolset)</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Label="Configuration" Condition="'$(Configuration)|$(Platform)'=='Release|Win32'">
    <TargetVersion>$(UMTargetVersion)</TargetVersion>
	<UseDebugLibraries>False</UseDebugLibraries>
    <PlatformToolset>$(UMToolset)</PlatformToolset>
  </PropertyGroup>
  <PropertyGroup Label="Configuration" Condition="$(TargetPlatformVersion.Split('.').Length)=='4'">
    <TargetPlatformMajorVersion>$(TargetPlatformVersion.Split(".")[0])</TargetPlatformMajorVersion>
    <TargetPlatformMinorVersion>$(TargetPlatformVersion.Split(".")[1])</TargetPlatformMinorVersion>
    <TargetPlatformRevVersion>$(TargetPlatformVersion.Split(".")[2])</TargetPlatformRevVersion>
    <TargetPlatformBuildVersion>$(TargetPlatformVersion.Split(".")[3])</TargetPlatformBuildVersion>
  </PropertyGroup>
  <PropertyGroup Label="Configuration" Condition="$(TargetPlatformVersion.Split('.').Length)!='4'">
    <TargetPlatformMajorVersion>0</TargetPlatformMajorVersion>
    <TargetPlatformMinorVersion>0</TargetPlatformMinorVersion>
    <TargetPlatformRevVersion>0</TargetPlatformRevVersion>
    <TargetPlatformBuildVersion>0</TargetPlatformBuildVersion>
  </PropertyGroup>
  <Choose>
     <When Condition="$(Configuration.Contains('Win'))">
         <PropertyGroup>
            <BUILD_ALT_DIR>$(Configuration.Replace(' ',''))</BUILD_ALT_DIR>
         </PropertyGroup>
     </When>
     <When Condition="'$(TargetVersion)'=='WindowsV6.3' Or '$(TargetVersion)'=='windowsV6.3'">
         <PropertyGroup>
            <BUILD_ALT_DIR>Win8.1$(Configuration)</BUILD_ALT_DIR>
         </PropertyGroup>
     </When>
      <When Condition="('$(TargetVersion)'=='Windows10' Or '$(TargetVersion)'=='windows10') And ('$(TargetPlatformMajorVersion)'>='10'  and  '$(TargetPlatformMinorVersion)'>='0' And '$(TargetPlatformRevVersion)'>='16232' And '$(TargetPlatformBuildVersion)'>='0' And $(TargetPlatformRevVersion) &lt; 17733)">
         <PropertyGroup>
            <BUILD_ALT_DIR>WinRS3$(Configuration)</BUILD_ALT_DIR>
         </PropertyGroup>
     </When>
	 <When Condition="('$(TargetVersion)'=='Windows10' Or '$(TargetVersion)'=='windows10') And ('$(TargetPlatformMajorVersion)'>='10'  and  '$(TargetPlatformMinorVersion)'>='0' And '$(TargetPlatformRevVersion)'>='17733' And '$(TargetPlatformBuildVersion)'>='0' And $(TargetPlatformRevVersion) &lt; 18305)">
         <PropertyGroup>
            <BUILD_ALT_DIR>Win2019$(Configuration)</BUILD_ALT_DIR>
         </PropertyGroup>
     </When>
	 <When Condition="('$(TargetVersion)'=='Windows10' Or '$(TargetVersion)'=='windows10') And ('$(TargetPlatformMajorVersion)'>='10'  and  '$(TargetPlatformMinorVersion)'>='0' And '$(TargetPlatformRevVersion)'>='18305' And '$(TargetPlatformBuildVersion)'>='0')">
         <PropertyGroup>
            <BUILD_ALT_DIR>Win19H1$(Configuration)</BUILD_ALT_DIR>
         </PropertyGroup>
     </When>
	 <Otherwise>
	    <PropertyGroup>
            <BUILD_ALT_DIR>$(TargetVersion.Replace('Windows','Win'))$(Configuration)</BUILD_ALT_DIR>
         </PropertyGroup>
	 </Otherwise>
 </Choose>
    <ItemDefinitionGroup Condition="('$(TargetPlatformMajorVersion)'>='10'  and  '$(TargetPlatformMinorVersion)'>='0' And '$(TargetPlatformRevVersion)'>='19546' And '$(TargetPlatformBuildVersion)'>='0')">  
        <ClCompile>
            <AdditionalOptions> -d2FH4- %(AdditionalOptions)</AdditionalOptions>
        </ClCompile>
        <Link>
            <AdditionalOptions >-d2:-FH4- %(AdditionalOptions)</AdditionalOptions>
        </Link>
  </ItemDefinitionGroup>
  <ItemDefinitionGroup>
    <Link><SetChecksum>true</SetChecksum></Link>
    <Link><AdditionalOptions>/DEBUG %(AdditionalOptions)</AdditionalOptions></Link>
  </ItemDefinitionGroup> 
  <PropertyGroup Label="PropertySheets">
    <ConversionToolVersion>1.0</ConversionToolVersion>
	<TARGETPATH Condition="'$(OVERRIDE_TARGETPATH)'!='true'">$(SolutionDir)\bin\user\obj$(BUILD_ALT_DIR)</TARGETPATH>
    <IntDir Condition="'$(Platform)'!='Win32'">$(BUILD_ALT_DIR)\$(Platform)\</IntDir>
    <IntDir Condition="'$(Platform)'=='Win32'">$(BUILD_ALT_DIR)\x86\</IntDir>
    <OutDir>$(IntDir)</OutDir>
  </PropertyGroup>
</Project>

