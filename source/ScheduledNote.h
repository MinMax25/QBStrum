#pragma once

namespace MinMax
{
	class ScheduledNote
	{
	public:
		bool valid = false;

		bool isSendNoteOn = false;

		uint64 onTime = 0;
		uint64 offTime = 0;

		int pitch = 0;
		int noteId = 0;
		int channel = 0;

		float velocity = 0.0f;

		std::string toString() const
		{
			char buf[256];
			std::snprintf(
				buf, sizeof(buf),
				"ScheduledNote{on=%llu off=%llu pitch=%d id=%d sent=%d vel=%.2f}",
				onTime, offTime, pitch, noteId, isSendNoteOn, velocity
			);
			return buf;
		}
	};
}