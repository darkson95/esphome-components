/*
 Copyright (C) 2021-2022 Fredrik Öhrström (gpl-3.0-or-later)

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include"meters_common_implementation.h"
#include"manufacturer_specificities.h"

namespace
{
    struct Driver : public virtual MeterCommonImplementation
    {
        Driver(MeterInfo &mi, DriverInfo &di);
    };

    static bool ok = registerDriver([](DriverInfo&di)
    {
        di.setName("qheat");
        di.setDefaultFields("name,id,total_energy_consumption_kwh,last_month_date,last_month_energy_consumption_kwh,timestamp");
        di.setMeterType(MeterType::HeatMeter);
        di.addLinkMode(LinkMode::C1);
        di.addDetection(MANUFACTURER_QDS, 0x04,  0x23);
        di.addDetection(MANUFACTURER_QDS, 0x04,  0x46);
        di.addDetection(MANUFACTURER_QDS, 0x37,  0x23);
        di.setConstructor([](MeterInfo& mi, DriverInfo& di){ return std::shared_ptr<Meter>(new Driver(mi, di)); });
    });

    Driver::Driver(MeterInfo &mi, DriverInfo &di) :  MeterCommonImplementation(mi, di)
    {
        addStringFieldWithExtractorAndLookup(
            "status",
            "Meter status.",
            DEFAULT_PRINT_PROPERTIES  | PrintProperty::STATUS | PrintProperty::INCLUDE_TPL_STATUS,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::ErrorFlags),
            {
                {
                    {
                        "ERROR_FLAGS",
                        Translate::MapType::BitToString,
                        AlwaysTrigger, MaskBits(0xffff),
                        "OK",
                        {
                            // !!! Uncertain !!!
                            // The documentation links to some intranet site, Qundis Error Codes Specification v1.7
                            // at https://base/svn/sys/Meter/Gen/G5-5/PUBL/Gen55_SysSpec_Error-Codes_EN_v1.6_any.pdf

                            // Here is a table that might apply:
                            // https://www.manualslib.com/manual/2046543/Qundis-Q-Heat-5-5-Us.html?page=5

                            { 0x01, "NO_FLOW" }, // F0
                            { 0x02, "SUPPLY_SENSOR_INTERRUPTED" }, // F1
                            { 0x04, "RETURN_SENSOR_INTERRUPTED" }, // F2
                            { 0x08, "TEMPERATURE_ELECTRONICS_ERROR" }, // F3
                            { 0x10, "BATTERY_VOLTAGE_ERROR" }, // F4
                            { 0x20, "SHORT_CIRCUIT_SUPPLY_SENSOR" }, // F5
                            { 0x40, "SHORT_CIRCUIT_RETURN_SENSOR" }, // F6
                            { 0x80, "MEMORY_ERROR" }, // F7
                            { 0x100, "SABOTAGE" }, // F8 - F1,2,3,5,6 longer than 8 hours, latching error, no more measurements performed.
                            { 0x200, "ELECTRONICS_ERROR" }, // F9
                        }
                    },
                },
            });

        addNumericFieldWithExtractor(
            "total_energy_consumption",
            "The total energy consumption recorded by this meter.",
             DEFAULT_PRINT_PROPERTIES,
            Quantity::Energy,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::AnyEnergyVIF)
            );

        addStringFieldWithExtractor(
            "last_month_date",
             "Last day previous month when total energy consumption was recorded.",
             DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(StorageNr(17))
            .set(VIFRange::Date)
            );

        addNumericFieldWithExtractor(
            "last_month_energy_consumption",
            "The total energy consumption recorded at the last day of the previous month.",
             DEFAULT_PRINT_PROPERTIES,
            Quantity::Energy,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(StorageNr(17))
            .set(VIFRange::AnyEnergyVIF)
            );

        addStringFieldWithExtractor(
            "last_year_date",
             "Last day previous year when total energy consumption was recorded.",
            DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(StorageNr(1))
            .set(VIFRange::Date)
            );

        addNumericFieldWithExtractor(
            "last_year_energy_consumption",
            "The total energy consumption recorded at the last day of the previous year.",
            DEFAULT_PRINT_PROPERTIES,
            Quantity::Energy,
            VifScaling::Auto, DifSignedness::Signed,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(StorageNr(1))
            .set(VIFRange::AnyEnergyVIF)
            );

        addStringFieldWithExtractor(
            "device_date_time",
             "Device date time.",
            DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::Instantaneous)
            .set(VIFRange::DateTime)
            );

        addStringFieldWithExtractor(
            "device_error_date",
             "Device error date.",
            DEFAULT_PRINT_PROPERTIES,
            FieldMatcher::build()
            .set(MeasurementType::AtError)
            .set(VIFRange::Date)
            );
    }
}

// Test: QHeato qheat 67228058 NOKEY
// telegram=|3C449344957002372337725880226793442304DC0000200C05043900004C0500000000426C9F2CCC080551070000C2086CBE29326CFFFF046D280DB62A|
// {"_":"telegram","media":"heat","meter":"qheat","name":"QHeato","id":"67228058","status":"OK","total_energy_consumption_kwh":390.4,"last_month_date":"2021-09-30","last_month_energy_consumption_kwh":75.1,"last_year_date":"2020-12-31","last_year_energy_consumption_kwh":0,"device_date_time":"2021-10-22 13:40","device_error_date":"2127-15-31","timestamp":"1111-11-11T11:11:11Z"}
// |QHeato;67228058;390.4;2021-09-30;75.1;1111-11-11 11:11.11


// Test: Qheatoo qheat 67506579 NOKEY
// telegram=|41449344796550674637727965506793444604dc0000200c0d000000004c0d00000000426cffffcc080d00000000c2086cdf2802fd170000326cffff046d3a0ddb29|
// {"_":"telegram","media":"heat","meter":"qheat","name":"Qheatoo","id":"67506579","status":"OK","total_energy_consumption_kwh":0,"last_month_date":"2022-08-31","last_month_energy_consumption_kwh":0,"last_year_date":"2127-15-31","last_year_energy_consumption_kwh":0,"device_date_time":"2022-09-27 13:58","device_error_date":"2127-15-31","timestamp":"1111-11-11T11:11:11Z"}
// |Qheatoo;67506579;0;2022-08-31;0;1111-11-11 11:11.11

// Test: Qheatoo qheat 78563412 NOKEY
// telegram=|3C449344123456782337729876543293442304FE0000200C05682235004C0580253200426CDF2CCC080525153500C2086CFE24326CFFFF046D1811F225|
// {"_":"telegram","media":"heat","meter":"qheat","name":"Qheatoo","id":"32547698","status":"OK","total_energy_consumption_kwh":35226.8,"last_month_date":"2023-04-30","last_month_energy_consumption_kwh":35152.5,"last_year_date":"2022-12-31","last_year_energy_consumption_kwh":32258,"device_date_time":"2023-05-18 17:24","device_error_date":"2127-15-31","timestamp":"1111-11-11T11:11:11Z"}
// |Qheatoo;32547698;35226.8;2023-04-30;35152.5;1111-11-11 11:11.11

// Test: Qheatoo qheat 78563411 NOKEY
// Comment: Proprietary Q walk-by message, these telegrams currently can be matched only by the first id
// telegram=|5344934411345678233778077998765431934423040dff5f350082fe00005f0107c005ffff68223500df2c80253200fe24251535005c03030000000000af03f508e91e1d2efc236e1fa218fe142f046d1911f225|
// {"_":"telegram","media":"heat","meter":"qheat","name":"Qheatoo","id":"31547698","status":"OK","total_energy_consumption_kwh":35226.8,"last_month_date":"2023-04-30","last_month_energy_consumption_kwh":35152.5,"last_year_date":"2022-12-31","last_year_energy_consumption_kwh":32258,"device_date_time":"2023-05-18 17:25","timestamp":"1111-11-11T11:11:11Z"}
// |Qheatoo;31547698;35226.8;2023-04-30;35152.5;1111-11-11 11:11.11
