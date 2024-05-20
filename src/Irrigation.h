#include <map>
#include <ArduinoJson.h>

const int irrigationChannels = 8;

// Schedule program:
// StartTime (hh:mm or disabled)
// weight (0-200% in 10% increments)
// Days to run on (0-6, all, odd, even, interval (1-10))
// Channel durations (0-60 minutes in 1 minute increments)

// Enum to represent the days to run on
enum class IrrigationDaysToRun
{
    Sunday,
    Monday,
    Tuesday,
    Wednesday,
    Thursday,
    Friday,
    Saturday,
    All,
    Odd,
    Even,
    Every3days,
    Every4days,
    Every5days,
    Every6days,
    Every7days,
    Every8days,
    Every9days
};

// Class to represent an irrigation schedule
class IrrigationSchedule
{
public:
    // Constructor
    IrrigationSchedule()
    {
        startTime = 0;
        for (int i = 0; i < irrigationChannels; i++)
        {
            duration[i] = 0;
        }
        daysToRun = IrrigationDaysToRun::All;
        weight = 100;
    }

    // Method to add a channel duration to the schedule
    bool addChannelDuration(int channel, int duration)
    {
        if (channel < 0 || channel >= irrigationChannels)
        {
            return false;
        }
        if (duration < 0 || duration > 60)
        {
            return false;
        }
        this->duration[channel] = duration;

        return true;
    }

    // Method to get the start time of the schedule
    int getStartTime() const
    {
        return startTime;
    }

    // Method to get the start time of the schedule in hours
    int getStartTimeHours() const
    {
        return startTime / 60;
    }

    // Method to get the start time of the schedule in minutes
    int getStartTimeMinutes() const
    {
        return startTime % 60;
    }

    // Method to get the duration of a channel in the schedule
    int getChannelDuration(int channel) const
    {
        if (channel < 0 || channel >= irrigationChannels)
        {
            return -1;
        }
        return duration[channel];
    }

    // Method to set start time of the schedule
    void setStartTime(int startHours, int startMinutes)
    {
        startTime = startHours * 60 + startMinutes;
    }

    // Method to set days to run on
    void setDaysToRun(IrrigationDaysToRun daysToRun)
    {
        this->daysToRun = daysToRun;
    }

    // Method to get days to run on
    IrrigationDaysToRun getDaysToRun() const
    {
        return daysToRun;
    }

    // Method to set weight of the schedule
    void setWeight(int weight)
    {
        if (weight < 0 || weight > 200)
        {
            return;
        }
        this->weight = weight;
    }

    // Method to get weight of the schedule
    int getWeight() const
    {
        return weight;
    }

private:
    int startTime;
    int duration[irrigationChannels];
    IrrigationDaysToRun daysToRun;
    int weight;
};

// Class to represent the irrigation schedules list
class IrrigationSchedules
{
public:
    // Method to add a schedule to the list
    void addSchedule(IrrigationSchedule schedule)
    {
        schedules[schedules.size()] = schedule;
    }

    // Mothod to update a schedule in the list
    void updateSchedule(int index, IrrigationSchedule schedule)
    {
        schedules[index] = schedule;
    }

    // Method to get the number of schedules in the list
    int getNumberOfSchedules() const
    {
        return schedules.size();
    }

    // Method to get a schedule from the list
    IrrigationSchedule getSchedule(int index) const
    {
        auto item = schedules.find(index);
        if (item == schedules.end())
        {
            return IrrigationSchedule();
        }
        return item->second;
    }

    // Method to remove a schedule from the list
    void removeSchedule(int index)
    {
        auto item = schedules.find(index);
        if (item != schedules.end())
        {
            schedules.erase(item);
        }
    }

private:
    std::map<int, IrrigationSchedule> schedules;
};

String convertToJson(const IrrigationSchedules &schedules) {
    DynamicJsonDocument doc(1024);
    JsonArray schedulesArray = doc.createNestedArray("schedules");
    for (int i = 0; i < schedules.getNumberOfSchedules(); i++) {
        IrrigationSchedule schedule = schedules.getSchedule(i);
        JsonObject scheduleObject = schedulesArray.createNestedObject();
        scheduleObject["startTime"] = schedule.getStartTime();
        scheduleObject["daysToRun"] = static_cast<int>(schedule.getDaysToRun());
        scheduleObject["weight"] = schedule.getWeight();
        JsonArray channelDurationsArray = scheduleObject.createNestedArray("channelDurations");
        for (int j = 0; j < irrigationChannels; j++) {
            int duration = schedule.getChannelDuration(j);
            channelDurationsArray.add(duration);
        }
    }
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}

bool convertFromJson(const String &jsonString, IrrigationSchedules &schedules) {
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.print(F("deserializeJson() failed: "));
        Serial.println(error.f_str());
        return false;
    }
    schedules = IrrigationSchedules();
    JsonArray schedulesArray = doc["schedules"].as<JsonArray>();
    for (JsonObject scheduleObject : schedulesArray) {
        IrrigationSchedule schedule;
        int startTime = scheduleObject["startTime"];
        schedule.setStartTime(startTime / 60, startTime % 60);
        schedule.setDaysToRun(static_cast<IrrigationDaysToRun>(scheduleObject["daysToRun"].as<int>()));
        schedule.setWeight(scheduleObject["weight"]);
        JsonArray channelDurationsArray = scheduleObject["channelDurations"].as<JsonArray>();
        for (int i = 0; i < irrigationChannels; i++) {
            schedule.addChannelDuration(i, channelDurationsArray[i].as<int>());
        }
        schedules.addSchedule(schedule);
    }

    return true;
}