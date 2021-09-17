#pragma once
//  ***************************************************************
//  TexturePacker - Creation date: 03/25/2009
//  -------------------------------------------------------------
//  Robinson Technologies Copyright (C) 2009 - All Rights Reserved
//
//  ***************************************************************
//  Programmer(s):  Seth A. Robinson (seth@rtsoft.com), Aki Koskinen
//  ***************************************************************

#ifndef TexturePacker_h__
#define TexturePacker_h__
#include "App.h"

class TexturePacker
{
public:
	TexturePacker();
	virtual ~TexturePacker();

	bool ProcessTexture(std::string fName);
	void SetCompressionType(pvrtexlib::PixelType pix) { m_pixType = pix; }
	void SetAlphaFix(bool on) { m_bAlphaFix = on; }
	bool UseAlphaFix() { return m_bAlphaFix; }
protected:

	bool m_bUsesTransparency;
	bool m_bAlphaFix;
	pvrtexlib::PixelType m_pixType;

private:
};


#endif // TexturePacker_h__