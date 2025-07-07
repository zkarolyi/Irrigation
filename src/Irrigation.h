#ifndef IRRIGATION_H
#define IRRIGATION_H

#include <map>
#include <vector>
#include <ArduinoJson.h>
#include <RTClib.h>
#include "settings.h"

const int irrigationChannelNumber = 8;

// Schedule program:
// StartTime (hh:mm or disabled)
// weight (0-200% in 10% increments)
// Days to run on (0-6, all, odd, even, interval (1-10))
// Channel durations (0-90 minutes in 1 minute increments)

const std::vector<const char*> daysToRunValues = {
    "All",
    "Odd",
    "Even",
    "Every3days",
    "Every4days",
    "Every5days",
    "Every6days",
    "Every7days"};

// Class to represent an irrigation schedule
class IrrigationSchedule
{
public:
    // Constructor
    IrrigationSchedule();
    // Method to add a channel duration to the schedule
    bool addChannelDuration(int channel, int duration);
    // Method to get the start time of the schedule
    int getStartTime() const;
    // Method to get the start time of the schedule in hours
    int getStartTimeHours() const;
    // Method to get the start time of the schedule in minutes
    int getStartTimeMinutes() const;
    // Method to get the start time of the schedule as a string
    String getStartTimeString() const;
    // Method to get the duration of a channel in the schedule
    int getChannelDuration(int channel) const;
    // Method to set start time of the schedule
    void setStartTime(int startHours, int startMinutes);
    // Method to set days to run on
    void setDaysToRun(int daysToRun);
    // Method to get days to run on
    int getDaysToRun() const;
    // Method to set weight of the schedule
    void setWeight(int weight);
    // Method to get weight of the schedule
    int getWeight() const;
    // Method to check if schedule is valid for the given day
    bool isValidForDay(DateTime date) const;

private:
    int startTime;
    int duration[irrigationChannelNumber];
    int daysToRun;
    int weight;
};

// Class to represent the irrigation schedules list
class IrrigationSchedules
{
public:
    // Constructor to initialize relay pins
    IrrigationSchedules();
    // Method to set the relay pins
    void setPins(const std::vector<int> &pins);
    // Method to get the relay pin for a given channel
    int getPin(int channel) const;
    // Method to add a schedule to the list
    void addSchedule(IrrigationSchedule schedule);
    // Mothod to update a schedule in the list
    void updateSchedule(int index, IrrigationSchedule schedule);
    // Method to get the number of channels in the schedule
    int getNumberOfChannels() const;
    // Method to get the number of schedules in the list
    int getNumberOfSchedules() const;
    // Method to get a schedule from the list
    IrrigationSchedule getSchedule(int index) const;
    // Method to remove a schedule from the list
    void removeSchedule(int index);
    // Method to remove all schedules
    void clearSchedules();

private:
    std::map<int, IrrigationSchedule> schedules;
    int irrigationRelayPins[irrigationChannelNumber];
};

bool isValidForDayRun(DateTime date, int dayToRun);
String convertToJson(const IrrigationSchedules &schedules);
bool convertFromJson(const String &jsonString, IrrigationSchedules &schedules);

#endif // IRRIGATION_H