#ifndef __TA3D_SDLCONTEXT_H__
#define __TA3D_SDLCONTEXT_H__

#include <stdexcept>
#include <memory>
#include "ta3d_string.h"

namespace TA3D
{
	String formatSdlVersion(const SDL_version version);

	/**
	 * Prints information about the compiled and linked versions of SDL
	 * and supporting SDL libraries to the log.
	 *
	 * This function can be called regardless of whether
	 * SDL or supporting libraries have been initialised.
	 */
	void logSdlVersions();

	class SDLException : public std::runtime_error
	{
	public:
		SDLException(const char* sdlError);
	};

	class SDLNetException : public std::runtime_error
	{
	public:
		SDLNetException(const char* sdlError);
	};

	class SDLMixerException : public std::runtime_error
	{
	public:
		SDLMixerException(const char* sdlError);
	};

	class SDLImageException : public std::runtime_error
	{
	public:
		SDLImageException(const char* sdlError);
	};

	class SdlContext
	{
	private:
		SdlContext();
		SdlContext(const SdlContext&) = delete;
		~SdlContext();

		friend class SdlContextManager;
	};

	class SdlNetContext
	{
	private:
		SdlNetContext();
		SdlNetContext(const SdlNetContext&) = delete;
		~SdlNetContext();

		friend class SdlContextManager;
	};

	class SdlMixerContext
	{
	private:
		SdlMixerContext();
		SdlMixerContext(const SdlMixerContext&) = delete;
		~SdlMixerContext();

		friend class SdlContextManager;
	};

	class SdlImageContext
	{
	private:
		SdlImageContext();
		SdlImageContext(const SdlImageContext&) = delete;
		~SdlImageContext();

		friend class SdlContextManager;
	};

	/**
	 * Manages the lifetime of SDL contexts.
	 * Contexts accessed via this manager
	 * are only valid for the lifetime of the manager.
	 *
	 * SDL is a global resource so don't instantiate more than one of these.
	 */
	class SdlContextManager
	{
	public:
		SdlContextManager();
		SdlContextManager(const SdlContextManager&) = delete;

		const SdlContext* getSdlContext() const;
		const SdlNetContext* getSdlNetContext() const;
		const SdlMixerContext* getSdlMixerContext() const;
		const SdlImageContext* getSdlImageContext() const;

	private:
		SdlContext sdlContext;
		SdlNetContext sdlNetContext;
		SdlMixerContext sdlMixerContext;
		SdlImageContext sdlImageContext;
	};
}


#endif // __TA3D_SDLCONTEXT_H__
