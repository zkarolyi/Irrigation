#include "Irrigation.h"

// ----------------------------------------
// Constructor for IrrigationSchedule class
// ----------------------------------------
IrrigationSchedule::IrrigationSchedule()
{
    startTime = 0;
    for (int i = 0; i < irrigationChannelNumber; i++)
    {
        duration[i] = 0;
    }
    daysToRun = 0;
    weight = 100;
}

// Add a channel duration
bool IrrigationSchedule::addChannelDuration(int channel, int duration)
{
    if (channel < 0 || channel >= irrigationChannelNumber || duration < manualIrrigationDurationMin || duration > manualIrrigationDurationMax)
    {
        return false;
    }
    this->duration[channel] = duration;
    return true;
}

// Get start time in minutes
int IrrigationSchedule::getStartTime() const
{
    return startTime;
}

// Get start time in hours
int IrrigationSchedule::getStartTimeHours() const
{
    return startTime / 60;
}

// Get start time in minutes
int IrrigationSchedule::getStartTimeMinutes() const
{
    return startTime % 60;
}

String IrrigationSchedule::getStartTimeString() const
{
    String startTimeString = (getStartTimeHours() < 10 ? "0" : "") + String(getStartTimeHours()) + ":" + (getStartTimeMinutes() < 10 ? "0" : "") + String(getStartTimeMinutes());
    return startTimeString;
}

// Get channel duration
int IrrigationSchedule::getChannelDuration(int channel) const
{
    if (channel < 0 || channel >= irrigationChannelNumber)
    {
        return -1;
    }
    return duration[channel];
}

// Set start time
void IrrigationSchedule::setStartTime(int startHours, int startMinutes)
{
    startTime = startHours * 60 + startMinutes;
}

// Set days to run
void IrrigationSchedule::setDaysToRun(int daysToRun)
{
    this->daysToRun = daysToRun;
}

// Get days to run
int IrrigationSchedule::getDaysToRun() const
{
    return daysToRun;
}

// Set weight
void IrrigationSchedule::setWeight(int weight)
{
    if (weight < 0 || weight > 200)
    {
        return;
    }
    this->weight = weight;
}

// Get weight
int IrrigationSchedule::getWeight() const
{
    return weight;
}

// Check if schedule is valid for the given day
bool IrrigationSchedule::isValidForDay(DateTime date) const
{
    return isValidForDayRun(date, daysToRun);
}

// -----------------------------------------
// Constructor for IrrigationSchedules class
// -----------------------------------------
IrrigationSchedules::IrrigationSchedules()
{}

void IrrigationSchedules::setPins(const std::vector<int> &pins)
{
    int i = 0;
    for (int pin : pins)
    {
        if (i < irrigationChannelNumber)
        {
            irrigationRelayPins[i++] = pin;
        }
    }
}

// Get relay pin for a channel
int IrrigationSchedules::getPin(int channel) const
{
    if (channel >= 0 && channel < irrigationChannelNumber)
    {
        return irrigationRelayPins[channel];
    }
    return -1;  // Invalid channel
}

// Add a schedule
void IrrigationSchedules::addSchedule(IrrigationSchedule schedule)
{
    schedules[schedules.size()] = schedule;
}

// Update a schedule
void IrrigationSchedules::updateSchedule(int index, IrrigationSchedule schedule)
{
    schedules[index] = schedule;
}

// Get the number of channels
int IrrigationSchedules::getNumberOfChannels() const
{
    return irrigationChannelNumber;
}

// Get the number of schedules
int IrrigationSchedules::getNumberOfSchedules() const
{
    return schedules.size();
}

// Get a schedule
IrrigationSchedule IrrigationSchedules::getSchedule(int index) const
{
    auto item = schedules.find(index);
    if (item == schedules.end())
    {
        return IrrigationSchedule();
    }
    return item->second;
}

// Remove a schedule
void IrrigationSchedules::removeSchedule(int index)
{
    auto item = schedules.find(index);
    if (item != schedules.end())
    {
        schedules.erase(item);
    }
}

// Clear all schedules
void IrrigationSchedules::clearSchedules()
{
    schedules.clear();
}

// Check if the schedule is valid for the given day
bool isValidForDayRun(DateTime date, int dayToRun)
{
   if (dayToRun == 0)  // All days
    {
        return true;
    }
    int day = date.unixtime() / 86400; // get number of days since epoch

    if (dayToRun == 1)  // Odd days
    {
        return day % 2 != 0;
    }
    if (dayToRun >= 2 && dayToRun <= 7)  // Every N days
    {
        return day % (dayToRun) == 0;
    }
    return false;
}

// Convert schedules to JSON
String convertToJson(const IrrigationSchedules &schedules)
{
    JsonDocument doc;
    JsonArray schedulesArray = doc["schedules"].to<JsonArray>();
    
    for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
    {
        IrrigationSchedule schedule = schedules.getSchedule(i);
        JsonObject scheduleObject = schedulesArray.add<JsonObject>();
        scheduleObject["startTime"] = schedule.getStartTime();
        scheduleObject["startTimeString"] = String(schedule.getStartTimeHours()) + ":" + String(schedule.getStartTimeMinutes());
        scheduleObject["daysToRun"] = static_cast<int>(schedule.getDaysToRun());
        scheduleObject["daysToRunString"] = daysToRunValues[schedule.getDaysToRun()];
        scheduleObject["weight"] = schedule.getWeight();
        
        JsonArray channelDurationsArray = scheduleObject["channelDurations"].to<JsonArray>();
        for (int j = 0; j < irrigationChannelNumber; j++)
        {
            channelDurationsArray.add(schedule.getChannelDuration(j));
        }
    }
    
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

// Convert JSON to schedules
bool convertFromJson(const String &jsonString, IrrigationSchedules &schedules)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, jsonString);
    
    if (error)
    {
        Serial.println(F("deserializeJson() failed"));
        return false;
    }

    schedules.clearSchedules();
    JsonArray schedulesArray = doc["schedules"].as<JsonArray>();
    
    for (JsonObject scheduleObject : schedulesArray)
    {
        IrrigationSchedule schedule;
        int startTime = scheduleObject["startTime"];
        schedule.setStartTime(startTime / 60, startTime % 60);
        schedule.setDaysToRun(scheduleObject["daysToRun"].as<int>());
        schedule.setWeight(scheduleObject["weight"]);
        
        JsonArray channelDurationsArray = scheduleObject["channelDurations"].as<JsonArray>();
        for (int i = 0; i < irrigationChannelNumber; i++)
        {
            schedule.addChannelDuration(i, channelDurationsArray[i].as<int>());
        }
        schedules.addSchedule(schedule);
    }

    return true;
}

// int dayOfYear(DateTime dt) {
//     static const int daysPerMonth[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };
//     int num = dt.day();
//     for (int i = 0; i < dt.month() - 1; i++) {
//         num += daysPerMonth[i];
//     }
//     if (dt.month() > 2 && dt.year() % 4 == 0 && 
//        (dt.year() % 100 != 0 || dt.year() % 400 == 0)) {
//         num += 1;
//     }

//     return num;
// }