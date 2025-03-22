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
    if (channel < 0 || channel >= irrigationChannelNumber || duration < 0 || duration > 60)
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

// Convert schedules to JSON
String convertToJson(const IrrigationSchedules &schedules)
{
    DynamicJsonDocument doc(1024);
    JsonArray schedulesArray = doc.createNestedArray("schedules");
    
    for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
    {
        IrrigationSchedule schedule = schedules.getSchedule(i);
        JsonObject scheduleObject = schedulesArray.createNestedObject();
        scheduleObject["startTime"] = schedule.getStartTime();
        scheduleObject["startTimeString"] = String(schedule.getStartTimeHours()) + ":" + String(schedule.getStartTimeMinutes());
        scheduleObject["daysToRun"] = static_cast<int>(schedule.getDaysToRun());
        scheduleObject["daysToRunString"] = daysToRunValues[schedule.getDaysToRun()];
        scheduleObject["weight"] = schedule.getWeight();
        
        JsonArray channelDurationsArray = scheduleObject.createNestedArray("channelDurations");
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
    DynamicJsonDocument doc(1024);
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

// String convertToJson(const IrrigationSchedules &schedules)
// {
//     JsonDocument doc;
//     JsonArray schedulesArray = doc.createNestedArray("schedules");
//     for (int i = 0; i < schedules.getNumberOfSchedules(); i++)
//     {
//         IrrigationSchedule schedule = schedules.getSchedule(i);
//         JsonObject scheduleObject = schedulesArray.createNestedObject();
//         scheduleObject["startTime"] = schedule.getStartTime();
//         scheduleObject["startTimeString"] = (schedule.getStartTimeHours() < 10 ? "0" : "") + String(schedule.getStartTimeHours()) + ":" + (schedule.getStartTimeMinutes() < 10 ? "0" : "") + String(schedule.getStartTimeMinutes());
//         scheduleObject["daysToRun"] = static_cast<int>(schedule.getDaysToRun());
//         scheduleObject["daysToRunString"] = IrrigationDaysToRunToString(schedule.getDaysToRun());
//         scheduleObject["weight"] = schedule.getWeight();
//         JsonArray channelDurationsArray = scheduleObject.createNestedArray("channelDurations");
//         for (int j = 0; j < irrigationChannelNumber; j++)
//         {
//             int duration = schedule.getChannelDuration(j);
//             channelDurationsArray.add(duration);
//         }
//     }
//     String jsonString;
//     serializeJson(doc, jsonString);
//     return jsonString;
// }

// bool convertFromJson(const String &jsonString, IrrigationSchedules &schedules)
// {
//     JsonDocument doc;
//     DeserializationError error = deserializeJson(doc, jsonString);
//     if (error)
//     {
//         Serial.print(F("deserializeJson() failed: "));
//         Serial.println(error.f_str());
//         return false;
//     }

//     schedules.clearSchedules();
//     JsonArray schedulesArray = doc["schedules"].as<JsonArray>();
//     for (JsonObject scheduleObject : schedulesArray)
//     {
//         IrrigationSchedule schedule;
//         int startTime = scheduleObject["startTime"];
//         schedule.setStartTime(startTime / 60, startTime % 60);
//         schedule.setDaysToRun(static_cast<IrrigationDaysToRun>(scheduleObject["daysToRun"].as<int>()));
//         schedule.setWeight(scheduleObject["weight"]);
//         JsonArray channelDurationsArray = scheduleObject["channelDurations"].as<JsonArray>();
//         for (int i = 0; i < irrigationChannelNumber; i++)
//         {
//             schedule.addChannelDuration(i, channelDurationsArray[i].as<int>());
//         }
//         schedules.addSchedule(schedule);
//     }

//     return true;
// }
