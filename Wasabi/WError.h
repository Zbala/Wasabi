/*********************************************************************
******************* W A S A B I   E N G I N E ************************
copyright (c) 2016 by Hassan Al-Jawaheri
desc.: Wasabi Engine error reporter
*********************************************************************/

#pragma once

/*********************************************************************
Errors
*********************************************************************/
enum W_ERROR {
	W_EXIT = 0,
	W_SUCCEEDED = 1,
	W_WINDOWNOTCREATED = 2,
	W_CLASSREGISTERFAILED = 3,
	W_UNABLETOCREATEDEVICE = 4,
	W_UNABLETOGETRENDERTARGET = 5,
	W_UNABLETOGETBACKBUFFER = 6,
	W_UNABLETOCREATE2DTEXTURE = 7,
	W_UNABLETOCREATEBUFFER = 8,
	W_ERRORUNK = 9,
	W_SHADERCOMPILEERROR = 10,
	W_UNABLETOLOADSHADER = 11,
	W_NOTVALID = 12,
	W_LOADFAILURE = 13,
	W_UNABLETOCREATEVERTEXLAYOUT = 14,
	W_IMAGENOTVALID = 15,
	W_FILENOTFOUND = 16,
	W_UNABLETOCREATEMESH = 17,
	W_UNABLETOCREATESWAPCHAIN = 18,
	W_FAILEDTOCREATEDXGIFACTORY = 19,
	W_INVALIDSUBSET = 20,
	W_INVALIDPARAM = 21,
	W_UNABLETOLOCKBUFFER = 22,
	W_WINDOWMINIMIZED = 23,
	W_NOHEADERFOUND = 24,
	W_CORRUPTEDHEADER = 25,
	W_INVALIDPTR = 26,
	W_VERTICESORINDICESCOUNTINVALID = 27,
	W_UNABLETOCREATEAMESHBUFFER = 28,
	W_UNABLETOCREATED2DFACTORY = 29,
	W_UNABLETOCREATESOUNDDEVICE = 30,
	W_UNABLETOCREATESOUNDBUFFER = 31,
	W_PHYSICSNOTINITIALIZED = 32,
	W_INVALIDFILEFORMAT = 33
};

class WError {
public:
	W_ERROR m_error;
	WError(W_ERROR err);

	operator bool();
};
