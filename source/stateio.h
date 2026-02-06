//------------------------------------------------------------------------
// Copyright(c) 2025 MinMax.
//------------------------------------------------------------------------
#pragma once

#include <string>
#include <cstdint>

#include <base/source/fstreamer.h>
#include <pluginterfaces/base/ibstream.h>
#include <pluginterfaces/base/fplatform.h>

namespace MinMax {

	struct StateIO
	{
		explicit StateIO(Steinberg::IBStream* state)
			: streamer(state, kLittleEndian) {
		}

		// ---- primitive ----

		bool writeDouble(double v)
		{
			return streamer.writeDouble(v);
		}

		bool readDouble(double& v)
		{
			return streamer.readDouble(v);
		}

		bool writeInt32(std::int32_t v)
		{
			return streamer.writeInt32(v);
		}

		bool readInt32(std::int32_t& v)
		{
			return streamer.readInt32(v);
		}

		// ---- UTF-8 string (length + raw bytes) ----

		bool writeWString(const std::wstring& s)
		{
			const std::int32_t len = static_cast<std::int32_t>(s.size());
			if (!streamer.writeInt32(len)) return false;
			return (len == 0) || streamer.writeRaw(s.data(), len * 2);
		}

		bool readWString(std::wstring& out)
		{
			std::int32_t len = 0;
			if (!streamer.readInt32(len)) return false;

			if (len <= 0)
			{
				out.clear();
				return true;
			}

			// 安全弁
			if (len > 4096) return false;

			out.resize(static_cast<size_t>(len));
			return streamer.readRaw(out.data(), len * 2);
		}

	private:
		Steinberg::IBStreamer streamer;
	};

}