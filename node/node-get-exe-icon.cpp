#include <napi.h>

extern "C" {
#include <stdio.h>
#include "../get-exe-icon.h"
}

namespace getexeicon {
	Napi::Buffer<char> IconFromFileWrapped(const Napi::CallbackInfo& info);
	Napi::Buffer<char> IconFromPidWrapped(const Napi::CallbackInfo& info);
	Napi::Buffer<char> DefaultExeIconWrapped(const Napi::CallbackInfo& info);
	Napi::Object Init(Napi::Env env, Napi::Object exports);
}

struct IcoBufFinalizer
{
	void operator()(Napi::Env env, char *data, void *hint) {
		free(data);
	}
};

Napi::Buffer<char> getexeicon::IconFromFileWrapped(const Napi::CallbackInfo& info) 
{
	if (info.Length() < 1 || !info[0].IsString()) {
		Napi::TypeError::New(info.Env(), "string file path expected").ThrowAsJavaScriptException();
		return Napi::Buffer<char>::New(info.Env(), 0);
	}
	std::u16string path = info[0].As<Napi::String>().Utf16Value();

	bool allowEmbeddedPNGs = true;
	if (info.Length() >= 2 && info[1].IsBoolean()) {
		allowEmbeddedPNGs = info[1].As<Napi::Boolean>().Value();
	}

	DWORD icoBufLen = 0;
	PBYTE icoBuf = get_exe_icon_from_file_utf16((wchar_t *)path.c_str(), allowEmbeddedPNGs, &icoBufLen);

	if (!icoBuf) {
		Napi::Error::New(info.Env(), "iconFromFile failed").ThrowAsJavaScriptException();
		return Napi::Buffer<char>::New(info.Env(), 0);
	}

	IcoBufFinalizer f;
	return Napi::Buffer<char>::New<IcoBufFinalizer, void>(info.Env(), (char *)icoBuf, (size_t)icoBufLen, f, NULL);
}

Napi::Buffer<char> getexeicon::IconFromPidWrapped(const Napi::CallbackInfo& info) 
{
	if (info.Length() < 1 || !info[0].IsNumber()) {
		Napi::TypeError::New(info.Env(), "int pid expected").ThrowAsJavaScriptException();
		return Napi::Buffer<char>::New(info.Env(), 0);
	}
	int pid = info[0].As<Napi::Number>().Int32Value();

	bool allowEmbeddedPNGs = true;
	if (info.Length() >= 2 && info[1].IsBoolean()) {
		allowEmbeddedPNGs = info[1].As<Napi::Boolean>().Value();
	}

	DWORD icoBufLen = 0;
	PBYTE icoBuf = get_exe_icon_from_pid(pid, allowEmbeddedPNGs, &icoBufLen);

	if (!icoBuf) {
		Napi::Error::New(info.Env(), "iconFromPid failed").ThrowAsJavaScriptException();
		return Napi::Buffer<char>::New(info.Env(), 0);
	}

	IcoBufFinalizer f;
	return Napi::Buffer<char>::New<IcoBufFinalizer, void>(info.Env(), (char *)icoBuf, (size_t)icoBufLen, f, NULL);
}

Napi::Buffer<char> getexeicon::DefaultExeIconWrapped(const Napi::CallbackInfo& info) 
{
	bool allowEmbeddedPNGs = true;
	if (info.Length() >= 1 && info[0].IsBoolean()) {
		allowEmbeddedPNGs = info[0].As<Napi::Boolean>().Value();
	}

	DWORD icoBufLen = 0;
	PBYTE icoBuf = get_default_exe_icon(allowEmbeddedPNGs, &icoBufLen);

	if (!icoBuf) {
		Napi::Error::New(info.Env(), "defaultExeIcon failed").ThrowAsJavaScriptException();
		return Napi::Buffer<char>::New(info.Env(), 0);
	}

	IcoBufFinalizer f;
	return Napi::Buffer<char>::New<IcoBufFinalizer, void>(info.Env(), (char *)icoBuf, (size_t)icoBufLen, f, NULL);
}

Napi::Object Init(Napi::Env env, Napi::Object exports) {
	exports.Set("getIconFromFile", Napi::Function::New(env, getexeicon::IconFromFileWrapped));
	exports.Set("getIconFromPid", Napi::Function::New(env, getexeicon::IconFromPidWrapped));
	exports.Set("getDefaultExeIcon", Napi::Function::New(env, getexeicon::DefaultExeIconWrapped));
	return exports;
}

NODE_API_MODULE(getexeicon, Init)
