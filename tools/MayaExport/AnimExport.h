#pragma once

#include <maya\MPxFileTranslator.h>
#include <maya\MPxCommand.h>
#include <maya\MDagPathArray.h>

#include <String\Path.h>




class PotatoAnimExporter
{
public:
	PotatoAnimExporter();
	~PotatoAnimExporter();

	MStatus convert(const MArgList &args);

private:
	MStatus processArgs(const MArgList &args);

private:
	int32_t startFrame_;
	int32_t endFrame_;
	float32_t fps_;
	MString nodes_;

	core::Path<char> fileName_;
	core::Path<char> filePath_;

};



class AnimExporter : public MPxFileTranslator
{
public:
	AnimExporter();
	~AnimExporter();

	MStatus writer(const MFileObject& file, const MString& optionsString, FileAccessMode mode);

	bool haveWriteMethod() const;
	MString defaultExtension() const;

	static void* creator();

};


class AnimExporterCmd : public MPxCommand
{
public:
	AnimExporterCmd();
	~AnimExporterCmd();

	virtual MStatus doIt(const MArgList &args);

	static void* creator(void);
};
