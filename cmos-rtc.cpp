/*
 * CMOS Real-time Clock
 */

/*
 * STUDENT NUMBER: s1817455
 */

#include <infos/drivers/timer/rtc.h>

#include <infos/util/lock.h>
#include <arch/x86/pio.h>

using namespace infos::drivers;
using namespace infos::drivers::timer;
using namespace infos::util;
using namespace infos::arch::x86;

class CMOSRTC : public RTC {
public:
	static const DeviceClass CMOSRTCDeviceClass;

	const DeviceClass& device_class() const override
	{
		return CMOSRTCDeviceClass;
	}

	//Returns true if the RTC is in BCD mode by reading the CMOS
	bool is_bcd()
	{
		// Interrupts are disabled
		UniqueIRQLock l;

		// If bit 2 from status register B is 0 then the register values are in BCD
		if(get_register(0xB, 2) == 0)
			return true;
		else 
			return false;

	}

	//Converts a timepoint from BCD to binary values
	void convert_BCD_to_binary(RTCTimePoint& tp)
	{
		tp.seconds = ((tp.seconds >> 4) * 10) + (tp.seconds & 0xF);
		tp.minutes = ((tp.minutes >> 4) * 10) + (tp.minutes & 0xF);
		tp.hours = ((tp.hours >> 4) * 10) + (tp.hours & 0xF);
		tp.day_of_month = ((tp.day_of_month >> 4) * 10) + (tp.day_of_month & 0xF);
		tp.month = ((tp.month >> 4) * 10) + (tp.month & 0xF);
		tp.year = ((tp.year >> 4) * 10) + (tp.year & 0xF);
	}


	/**
	 * Interrogates the RTC to read the current date & time.
	 * @param tp Populates the tp structure with the current data & time, as
	 * given by the CMOS RTC device.
	 */
	void read_timepoint(RTCTimePoint& tp) override
	{
		tp = get_timepoint();
		RTCTimePoint prev = tp;

		// Avoids inconsistent values by reading timepoints until two of them are the same
		do {
			prev = tp;
			tp = get_timepoint();
		} while (!(tp.seconds == prev.seconds) && 
				  (tp.minutes == prev.minutes) && 
				  (tp.hours == prev.hours) && 
				  (tp.day_of_month == prev.day_of_month) && 
				  (tp.month == prev.month) && (tp.year == prev.year) && 
				  true);

			
		if (is_bcd()) {
			convert_BCD_to_binary(tp);
		}
	}


private:

	/**
	 * @warning It does not make sure if the interrupts are disabled
	 * Returns the data from the specified register
	 */
	uint8_t get_register(int reg)
	{
		__outb(0x70, reg); // activate the register
		return __inb(0x71); // CMOS Data
	}

	/**
	 * @warning Not suitable for batch operations
	 * Returns the specified bit at the given register from the CMOS
	 */
	uint8_t get_register(int reg, int bit)
	{
		return (get_register(reg) >> bit) & 1;
	}


	/**
	 * @warning It is computationally expensive
	 * Reads a timepoint from the RTC and returns the timepoint composed from several register reads
	 */
	RTCTimePoint get_timepoint()
	{
		// Interrupts are disabled 
		UniqueIRQLock l;
		auto bit = get_register(0xA, 7) == 0;

		// Wait for current update to complete
		while (bit != 0) {
			bit = get_register(0xA, 7) == 0;
		}

		// No update is in progress if the bit is not cleared
		return RTCTimePoint{
			.seconds = get_register(0x00),
			.minutes = get_register(0x02),
			.hours = get_register(0x04),
			.day_of_month = get_register(0x07),
			.month = get_register(0x08),
			.year = get_register(0x09),
		};
	}

};

const DeviceClass CMOSRTC::CMOSRTCDeviceClass(RTC::RTCDeviceClass, "cmos-rtc");

RegisterDevice(CMOSRTC);
