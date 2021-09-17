#include "PlatformPrecomp.h"
#include "App.h"
#include "util/MiscUtils.h"
#include "util/ResourceUtils.h"
#include "util/Variant.h"
#include "Manager/VariantDB.h"

#include "FileSystem/FileManager.h"
#include "util/TextScanner.h"
#include "FontPacker.h"
#include <fstream>
#include <sys/types.h>
#include <sys/stat.h>


extern MainHarness g_mainHarness;

FileManager g_fileManager;

FileManager* GetFileManager()
{
	return &g_fileManager;
}

App::App()
{

	m_bForceAlpha = false;
	m_bNoPowerOf2 = false;
	m_output = RTTEX;
	SetPixelType(pvrtexlib::PixelType(0));
	SetPixelTypeIfNotSquareOrTooBig(pvrtexlib::PixelType(0));
	SetMaxMipLevel(1);
	SetStretchImage(false);
	SetForceSquare(false);
	SetFlipV(false);
	m_ultraCompressQuality = 0;
}

App::~App()
{
}

bool App::Init()
{
	return true;
}

string App::GetPixelTypeText()
{
	return m_pixelTypeText;
}

void App::SetPixelTypeText(string s)
{
	m_pixelTypeText = s;
}

void App::SetPixelType(pvrtexlib::PixelType ptype)
{
	m_pixelType = ptype;

	switch (ptype)
	{
#ifndef RT_NO_PVR

	case pvrtexlib::OGL_PVRTC2:
		SetPixelTypeText("PVRTC2"); break;

	case pvrtexlib::OGL_PVRTC4:
		SetPixelTypeText("PVRTC4"); break;
#endif
	case pvrtexlib::OGL_RGBA_4444:
		SetPixelTypeText("RGBA 16 bit"); break;
	case pvrtexlib::OGL_RGBA_8888:
		SetPixelTypeText("RGBA 32 bit"); break;
	case pvrtexlib::OGL_RGB_565:
		SetPixelTypeText("RGB 16 bit (565)"); break;
	case pvrtexlib::OGL_RGB_888:
		SetPixelTypeText("RGB 24 bit"); break;

	default:

		SetPixelTypeText("Unknown");
	}
}



void ShowHelp()
{
	LogMsg("Help and examples\n");
	LogMsg("RTPack <any file> (Compresses it as an rtpack without changing the name)");

	LogMsg("");
	LogMsg("More options/flags for making textures:\n");
	LogMsg("RTPack -8888 <image file> (Creates raw rgba 32 bit .rttex, or 24 bit if no alpha");
	LogMsg("RTPack -8888 -ultra_compress 90 <image file> (Writes .rttex with jpg compression when there isn't alpha)");


	LogMsg("More extra flags you can use with texture generation:");
	LogMsg("-mipmaps (Causes mipmaps to be generated)");
	LogMsg("-stretch (Stretches instead of pads to reach power of 2)");
	LogMsg("-force_square (forces textures to be square in addition to being power of 2)");
	LogMsg("-8888_if_not_square_or_too_big (1024 width or height and non square will use -8888)");
	LogMsg("-flipv (vertical flip, applies to textures only)");
	LogMsg("-ultra_compress <0 to 100> (100 is best quality.  only applied to things that DON'T use alpha)");
	LogMsg("-nopowerof2 (stops rtpack from adjusting images to be power of 2)");
	LogMsg("-o <format> Writes final output as a normal image, useful for testing.  Formats can be: bmp jpg");

	LogMsg("-h2 (shows the modified help)");

	LogMsg("You can drag multiple rttex textures into the program and they will be unpacked back into png images");
	LogMsg("You can drag multiple png images into the program and they will be packed to rttex (same as -8888)");
	LogMsg("Alpha premultiplication colors are fixed by default, but you can use -8888 to not explicitly use it.");
	LogMsg("All dragged rttex files will also be automatically compressed, so you don't need to first rtpack -8888 n.png and then rtpack n.rttex");
	LogMsg("In order to explicitly only decompress rttex files use: -decomp");
	LogMsg("In order to explicitly disable automatic compression use: -nocomp");
}

#ifdef WINAPI
#include <direct.h>
string rtpack_dir()
{
	static std::string currdir = "";
	if (currdir == "") {
		TCHAR szDllName[_MAX_PATH];
		TCHAR szDrive[_MAX_DRIVE];
		TCHAR szDir[_MAX_DIR];
		TCHAR szFilename[256];
		TCHAR szExt[256];
		GetModuleFileName(0, szDllName, _MAX_PATH);
		_splitpath(szDllName, szDrive, szDir, szFilename, szExt);

		currdir = string(szDrive) + string(szDir);
	}

	return currdir;
}
#endif

bool is_enabled(std::string line) {
	return line == "enable" || line == "enabled" || line == "on" || line == "true" || line == "yes" || line == "active";
}

std::string get_dir_back(std::string dir, int count) { //how many folders to move back in 
	for (int i = 0; i < count; i++)
		dir = GetPathFromString(RemoveTrailingBackslash(dir));
	return dir;
}
bool App::Update()
{
	LogMsg("RTPack V1.6 (Unofficial by ama, original by Seth) [-h for help or -h2 for unofficial help]\n");

	//don't mind these, it's for my own usage so that i can automatically relocate packed files into git and build folders for private server
	if (FileExists(rtpack_dir() + "\\rtconfig.txt")) {
		std::ifstream in(rtpack_dir() + "\\rtconfig.txt");

		std::vector<std::string> settings{};
		int pos = 0;
		std::string str = "";
		while (std::getline(in, str))
		{
			if (str.size() > 1 && str.find("#") != 0)
				settings.push_back(str);
		}

		if (settings.size() < 8) {
			LogMsg("Found config file but the order/size is invalid");
			PressAnyKey();
			return false;
		}
		else {

			bool win = is_enabled(settings[pos++]);
			header_info = is_enabled(settings[pos++]);
			close_console = is_enabled(settings[pos++]);
			if (win) {
				update_rttex_git = is_enabled(settings[pos++]);
				update_rttex_release = is_enabled(settings[pos++]);

				if (update_rttex_git) {
					git_folder = get_dir_back(rtpack_dir(), 1) + "growtopia\\";
					struct stat info;
					bool error = stat(git_folder.c_str(), &info) != 0;
					if (error || !(!error && info.st_mode & S_IFDIR)) {
						LogMsg("Disabling git update relocation: Either no access to the folder or it doesn't exist");
						git_folder = "";
						update_rttex_git = false;
						PressAnyKey();
					}
				}

				if (update_rttex_release) {
					release_folder = get_dir_back(rtpack_dir(), 3) + "bin\\release\\growtopia\\";
					struct stat info;
					bool error = stat(release_folder.c_str(), &info) != 0;
					if (error || !(!error && info.st_mode & S_IFDIR)) {
						LogMsg("Disabling vs build relocation: Either no access to the folder or it doesn't exist");
						PressAnyKey();
						update_rttex_release = false;
						release_folder = "";
					}
				}
				delete_original = is_enabled(settings[pos++]);
				extra_logging = is_enabled(settings[pos++]);
			}
			else
				pos += 4; //skip relocate options in linux

			if (settings[pos++] != "align_check") {
				LogMsg("Config align check failed. Should have been align_check, got %s", settings[pos - 1].c_str());
				PressAnyKey();
				return false;
			}
			std::string curr_dir = "";
			for (int i = pos; i < settings.size(); i++) {

				auto line = settings[i];
				if (line.find("dir=") == 0) {
					StringReplace("dir=", "", line);
					curr_dir = line;
					continue;
				}
				relocation_paths[line] = curr_dir;
			}

			LogMsg("Loaded rtconfig");
		}
	}


	if (g_mainHarness.ParmExists("-h2") || g_mainHarness.m_parms.size() == 0) {
		LogMsg("You can drag multiple rttex textures into the program and they will be unpacked back into png images");
		LogMsg("You can drag multiple png images into the program and they will be packed to rttex (same as -8888)");
		LogMsg("Alpha premultiplication colors are fixed by default, but you can use -8888 to not explicitly use it.");
		LogMsg("All dragged rttex files will also be automatically compressed, so you don't need to first rtpack -8888 n.png and then rtpack n.rttex");
		LogMsg("In order to explicitly only decompress rttex files use: -decomp");
		LogMsg("In order to explicitly disable automatic compression use: -nocomp");
		PressAnyKey();
		return false;
	}

	bool decomp = g_mainHarness.ParmExists("-decomp");
	bool nocomp = g_mainHarness.ParmExists("-nocomp");

	string outputFormat;
	if (g_mainHarness.ParmExistsWithData("-o", &outputFormat))
	{
		if (outputFormat == "png")
		{
			GetApp()->SetOutput(App::PNG);
		}
		if (outputFormat == "jpg")
		{
			GetApp()->SetOutput(App::JPG);
		}
		if (outputFormat == "bmp")
		{
			GetApp()->SetOutput(App::BMP);
		}
	}

	string qualityLevel;
	if (g_mainHarness.ParmExistsWithData("-ultra_compress", &qualityLevel))
	{
		int quality = atoi(qualityLevel.c_str());
		if (quality < 1 || quality > 100)
		{
			LogMsg("ERROR:  -ultra_compress has invalid quality level set. Should be 1 to 100. Example: -ultracompress 70");
			WaitForKey();
			return false;
		}

		GetApp()->SetUltraCompressQuality(quality);
	}

	if (g_mainHarness.ParmExists("-mipmaps"))
		GetApp()->SetMaxMipLevel(20);

	if (g_mainHarness.ParmExists("-flipv"))
		GetApp()->SetFlipV(true);

	if (g_mainHarness.ParmExists("-stretch"))
		GetApp()->SetStretchImage(true);

	if (g_mainHarness.ParmExists("-force_square"))
		GetApp()->SetForceSquare(true);

	if (g_mainHarness.ParmExists("-8888_if_not_square_or_too_big"))
		GetApp()->SetPixelTypeIfNotSquareOrTooBig(pvrtexlib::OGL_RGBA_8888);

	if (g_mainHarness.ParmExists("-4444_if_not_square_or_too_big"))
		GetApp()->SetPixelTypeIfNotSquareOrTooBig(pvrtexlib::OGL_RGBA_4444);


	int argc = g_mainHarness.GetParmCount();
	GetApp()->SetForceAlpha(true);
	if (g_mainHarness.ParmExists("/h") || g_mainHarness.ParmExists("-h") || argc < 1)
		ShowHelp();

	if (g_mainHarness.ParmExists("-nopowerof2"))
		GetApp()->SetNoPowerOfTwo(true);

	if (g_mainHarness.ParmExists("-pvrt8888") || g_mainHarness.ParmExists("-8888"))
		GetApp()->SetPixelType(pvrtexlib::OGL_RGBA_8888);

	bool manual = g_mainHarness.ParmExists("-pvrt8888") || g_mainHarness.ParmExists("-8888") || GetApp()->GetPixelType() != 0;

	if (manual)
	{
		if (g_mainHarness.GetParmCount() < 2)
		{
			LogError("Aren't you missing the filename?");
		}
		else
		{
			TexturePacker packer;
			packer.SetCompressionType(GetApp()->GetPixelType());
			packer.ProcessTexture(g_mainHarness.GetLastParm());
		}
		return  false;
	}
	GetApp()->SetPixelType(pvrtexlib::OGL_RGBA_8888);
	for (auto param : g_mainHarness.m_parms) {
		if (param.find("-") == 0)
			continue;

		auto extension = GetFileExtension(param);
		auto filename = GetFileNameFromString(param);
		LogMsg("\nProcessing %s", filename.c_str());
		if (extension == "rttex") {
			if (decomp)
				DecompressAndSave(param);
			else
				Unpack(param);
		}
		else if (extension == "png") {
			TexturePacker packer;
			packer.SetAlphaFix(true);
			packer.SetCompressionType(GetApp()->GetPixelType());
			packer.ProcessTexture(param);
			std::string rttex_path = ModifyFileExtension(param, "rttex");
			
			if (!nocomp) //compress the file right after processing 
				CompressFile(rttex_path);

			auto fname = GetFileNameWithoutExtension(param);
			auto relocate = [&](auto folder) {
				if (relocation_paths.count(fname) > 0) {
					auto dest_dir = relocation_paths[fname];
					auto dest_file = folder + dest_dir + "\\" + fname + ".rttex";
					char src[1025] = { 0 };
					char dst[1025] = { 0 };
					StringReplace("/", "\\", dest_file);
					StringReplace("/", "\\", rttex_path);

					if (!CopyFile(rttex_path.c_str(), dest_file.c_str(), false)) {
						LogMsg("Relocation failed for some reason, here are the paths. Check if they exist?");
						LogMsg("src: %s", rttex_path.c_str());
						LogMsg("dst: %s", dest_file.c_str());
						PressAnyKey();
						return false;
					}
				}
				else if (extra_logging) {
					LogMsg("[extra logging option] Didn't find relocation path for %s", fname.c_str());
					PressAnyKey();
					return false;
				}
				return true;
			};


			auto path_str = GetPathFromString(param);
			//either  we used cmd with relative paths, and it matches the folder
			//or abs directory is same (dragged and dropped texture into the file probably)
			if ((path_str == param && extension == "png") || (path_str == rtpack_dir() && extension == "png"))
			{
				bool relocated = false;
				if (update_rttex_git)
					relocated = relocate(git_folder);

				if (update_rttex_release) {
					bool res = relocate(release_folder);
					if (!relocated)
						relocated = res;
				}
				if ((update_rttex_git || update_rttex_release) && relocated) {

					if (!update_rttex_release)
						LogMsg("Relocated to vs build folder (%s)", relocation_paths[fname].c_str());
					else if (!update_rttex_git)
						LogMsg("Relocated to git content folder (%s)", relocation_paths[fname].c_str());
					else
						LogMsg("Relocated to vs build and git content folders (%s)", relocation_paths[fname].c_str());

					if (delete_original && !DeleteFile(rttex_path.c_str())) {
						LogMsg("Failed to delete original rttex file (has been relocated), you have to manually delete it.");
						PressAnyKey();
					}
				}
			}



		}
		else
			LogMsg("Unknown extension %s", extension.c_str());
	}
	if (!close_console) {
		PressAnyKey();
	}
	return false; //false signals we're done
}

void App::Kill()
{
}
