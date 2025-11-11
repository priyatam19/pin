struct Sensor
{
    int id;
    double reading;
};

double read_sensor_value(const struct Sensor *sensor)
{
    if (!sensor) {
        return 0.0;
    }
    return sensor->reading;
}
