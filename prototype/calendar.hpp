#ifndef RCR_CALENDAR_DEFINITIONS
#define RCR_CALENDAR_DEFINITIONS

#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include <bitset>
#include <algorithm>

#include "common_utils.hpp"
#include "data_codecs.hpp"
#include "timeanddate.hpp"



enum RepeatType {
    NoRepeat = 'N',
    Daily    = 'D',
    Weekly   = 'W',
    Monthly  = 'M',
    Yearly   = 'Y',
};


struct TimeBlock {
    std::string name;
    TimeAndDate start;
    TimeAndDate end;
    RepeatType repeat_period;
    u32 repeat_count;
    
    inline TimeBlock() : start(TimeAndDate()), end(TimeAndDate()), repeat_period(NoRepeat), repeat_count(0) {}
    inline TimeBlock(TimeAndDate start, TimeAndDate end, RepeatType repeat_period, u32 repeat_count) : start(start), end(end), repeat_period(repeat_period), repeat_count(repeat_count) {
        if (this->end < this->start) {
            // Swap the end and the start if they are in the wrong order
            TimeAndDate temp = this->start;
            this->start = this->end;
            this->end = temp;
        }
    }
    
    inline bool is_valid() const {
        return this->start <= this->end;
    }
    
    inline bool overlaps(const TimeBlock& other) const {
        // Check if one time block starts before the other ends and ends after the other starts (inverse of having no overlap)
        return this->start < other.end && other.start < this->end;
    }

    inline bool within(const TimeBlock& other) const {
        return other.start <= this->start && other.end >= this->end;
    }
    
    
    static inline std::string encode_static(const TimeBlock& block) { return block.encode(); }
    std::string encode() const {
        std::ostringstream oss;
        oss << this->start.encode() << " " << this->end.encode() << " " << static_cast<char>(this->repeat_period) << " " << this->repeat_count;
        return oss.str();
    }
    
    static inline Status decode_static(std::istream& s, TimeBlock& block) { return block.decode(s); }
    Status decode(std::istream& s) {
        propagate(this->start.decode(s));
        propagate(this->end.decode(s));
        
        char c;
        s >> c;
        if (s.fail()) return Failure;
        this->repeat_period = static_cast<RepeatType>(c);
        switch (this->repeat_period) {
            case NoRepeat:
            case Daily:
            case Weekly:
            case Monthly:
            case Yearly:
                break;
            default:
                return Failure;
        }
        
        s >> this->repeat_count;
        if (s.fail()) return Failure;
        
        return Success;
    }
    
    std::string to_string() const {
        std::ostringstream oss;
        oss << "\n" << this->name << ":\nStart: " << this->start.to_string() << "\nEnd: " << this->end.to_string() << "\n";
        
        switch (this->repeat_period) {
            case NoRepeat: oss << "Does not repeat\n"; break;
            case Daily: oss << "Every day for " << this->repeat_count << " days\n"; break;
            case Weekly: oss << "Every week for " << this->repeat_count << " weeks\n"; break;
            case Monthly: oss << "Every month for " << this->repeat_count << " months\n"; break;
            case Yearly: oss << "Every year for " << this->repeat_count << " years\n"; break;
            default: oss << "Invalid repeat property\n";
        }
        
        return oss.str();
    }
    
};


struct Calendar {
    std::vector<TimeBlock> busy_times;
    
    inline Calendar() : busy_times(std::vector<TimeBlock>()) {}
    
    bool is_time_block_valid(const TimeBlock& block) const {
        // this might have to be reworked, a time block will always collide with itself if it is in the list
        // also blocks that overlap can still be valid if they repeat differently, most calendars allow for overlapping events
        for (const TimeBlock& other_block : this->busy_times) {
            if (block.overlaps(other_block)) {
                return false;
            }
        }
        
        return true;
    }
    
    inline std::vector<TimeBlock> get_busy_times_in_day(const TimeAndDate& day) {
        return this->get_busy_times_in_range(
            TimeAndDate::build(0, day.get_day_of_year(), day.get_year()),
            TimeAndDate::build(0, day.get_day_of_year() + 1, day.get_year())
        );
    }
    
    std::vector<TimeBlock> get_busy_times_in_range(const TimeAndDate& start, const TimeAndDate& end) {
        // todo: make work with repeating blocks
        let range = TimeBlock(start, end, NoRepeat, 0);
        std::vector<TimeBlock> out;
        for (const TimeBlock& busy_time : this->busy_times) {
            if (range.overlaps(busy_time)) {
                out.push_back(busy_time);
            }
        }
        return out;
    }
    
    void sort_busy_times() {
        std::sort(this->busy_times.begin(), this->busy_times.end(), [](const TimeBlock& a, const TimeBlock& b) {
            if (a.start != b.start) {
                return a.start < b.start;
            } else {
                return a.end < b.end;
            }
        });
    }
    
    
    static inline std::string encode_static(const Calendar& calendar) { return calendar.encode(); }
    std::string encode() const {
        return encode_vector<TimeBlock>(this->busy_times, TimeBlock::encode_static, true);
    }
    
    static inline Status decode_static(std::istream& s, Calendar& calendar) { return calendar.decode(s); }
    Status decode(std::istream& s) {
        return decode_vector<TimeBlock>(s, this->busy_times, TimeBlock::decode_static);
    }
    
    
    
};



#endif