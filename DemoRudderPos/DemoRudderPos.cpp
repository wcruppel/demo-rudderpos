#include <Windows.h>
#include <SimConnect.h>
#include <comdef.h>
#include <tchar.h>
#define _USE_MATH_DEFINES
#include <math.h>

#pragma comment (lib, "SimConnect.lib")

#define DEG_TO_RAD          (M_PI / 180.0)
#define RAD_TO_DEG          (180.0 / M_PI)

#define NM_TO_FT            (2315000.0 / 381.0)
#define FT_TO_NM            (381.0 / 2315000.0)

#define FT_TO_RAD           (FT_TO_NM * M_PI / 180.0 / 60.0)
#define RAD_TO_FT           (NM_TO_FT * 180.0 * 60.0 / M_PI)


class CDemoRudderPos
{
public:
    CDemoRudderPos () :
        m_hSimConnect        (NULL),
        m_bQuit              (false),
        m_idObjGroundVehicle (0),
        m_bDataUserObjectSet (false)
    {
    }

    void Run ()
    {
        HRESULT hr = SimConnect_Open (&m_hSimConnect, "DemoRudderPos", NULL, 0, 0, 0);
        if (SUCCEEDED (hr))
        {
            _tprintf (
                _T("Connected to sim! Press:\n")
                _T("  C to create the ground vehicle\n")
                _T("  A to move rudder left\n")
                _T("  D to move rudder right\n")
                _T("  X to quit\n")
            );

            // Create private events
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_CREATE);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_RUDDER_LEFT);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_RUDDER_RIGHT);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_QUIT);

            // Assign the private events to a notification group
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, NOTIFY_GROUP_ID_KEYBOARD, EVENT_ID_CREATE);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, NOTIFY_GROUP_ID_KEYBOARD, EVENT_ID_RUDDER_LEFT);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, NOTIFY_GROUP_ID_KEYBOARD, EVENT_ID_RUDDER_RIGHT);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, NOTIFY_GROUP_ID_KEYBOARD, EVENT_ID_QUIT);

            // Link the private events to keyboard keys
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, INPUT_GROUP_ID_KEYBOARD, "C", EVENT_ID_CREATE);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, INPUT_GROUP_ID_KEYBOARD, "A", EVENT_ID_RUDDER_LEFT);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, INPUT_GROUP_ID_KEYBOARD, "D", EVENT_ID_RUDDER_RIGHT);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, INPUT_GROUP_ID_KEYBOARD, "X", EVENT_ID_QUIT);

            // Turn on notifications for the private events
            SimConnect_SetInputGroupState (m_hSimConnect, NOTIFY_GROUP_ID_KEYBOARD, SIMCONNECT_STATE_ON);

            // Set up data definition for the user object
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID_USER_OBJECT,
                "PLANE LATITUDE",
                "degrees",
                SIMCONNECT_DATATYPE_FLOAT64
            );
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID_USER_OBJECT,
                "PLANE LONGITUDE",
                "degrees",
                SIMCONNECT_DATATYPE_FLOAT64
            );
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID_USER_OBJECT,
                "PLANE HEADING DEGREES TRUE",
                "degrees",
                SIMCONNECT_DATATYPE_FLOAT64
            );
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID_USER_OBJECT,
                "PLANE ALTITUDE",
                "feet",
                SIMCONNECT_DATATYPE_FLOAT64
            );

            // Request data on user object
            SimConnect_RequestDataOnSimObject (
                m_hSimConnect,
                DATA_REQ_ID_USER_OBJECT,
                DATA_DEF_ID_USER_OBJECT,
                SIMCONNECT_OBJECT_ID_USER,
                SIMCONNECT_PERIOD_ONCE
            );

            // Set up data definition for the ground vehicle
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID_GROUND_VEHICLE,
                "RUDDER POSITION",
                "position",
                SIMCONNECT_DATATYPE_FLOAT64
            );

            // Dispatch loop
            while (!m_bQuit)
            {
                SimConnect_CallDispatch (m_hSimConnect, DispatchProc_, this);
                Sleep (1);
            }

            SimConnect_Close (m_hSimConnect);
        }
        else
        {
            _com_error error (hr);
            _tprintf (_T("Failed to connect to sim: %s\n"), error.ErrorMessage ());
        }
    }

private:
    enum EVENT_ID
    {
        EVENT_ID_CREATE,
        EVENT_ID_RUDDER_RIGHT,
        EVENT_ID_RUDDER_LEFT,
        EVENT_ID_QUIT
    };

    enum DATA_REQ_ID
    {
        DATA_REQ_ID_USER_OBJECT,
        DATA_REQ_ID_GROUND_VEHICLE
    };

    enum DATA_DEF_ID
    {
        DATA_DEF_ID_USER_OBJECT,
        DATA_DEF_ID_GROUND_VEHICLE
    };

    enum NOTIFY_GROUP_ID
    {
        NOTIFY_GROUP_ID_KEYBOARD
    };

    enum INPUT_GROUP_ID
    {
        INPUT_GROUP_ID_KEYBOARD
    };

#pragma pack (push, 1)
    typedef struct DataUserObject
    {
        DataUserObject ()
        {
            dLat  = 0.0;
            dLon  = 0.0;
            dHead = 0.0;
            dAlt  = 0.0;
        }
        
        double dLat;
        double dLon;
        double dHead;
        double dAlt;
    }
    DataUserObject;

    typedef struct DataGroundVehicle
    {
        DataGroundVehicle ()
        {
            dRudderPos = 0.0;
        }

        double dRudderPos;
    }
    DataGroundVehicle;
#pragma pack (pop)


    void CALLBACK DispatchProc (SIMCONNECT_RECV* pData,
                                DWORD            cbData)
    {
        switch (pData->dwID)
        {
            case SIMCONNECT_RECV_ID_EVENT:
            {
                SIMCONNECT_RECV_EVENT* evt = (SIMCONNECT_RECV_EVENT*)pData;

                switch (evt->uEventID)
                {
                    case EVENT_ID_CREATE:
                    {
                        if (!m_bDataUserObjectSet)
                        {
                            _tprintf (_T("No data from user object yet!\n"));
                            break;
                        }
                        else if (m_idObjGroundVehicle)
                        {
                            _tprintf (_T("Ground vehicle already created!\n"));
                        }
                        else
                        {
                            // Align with user aircraft, but heading 90 degrees to the right so we can see wheels
                            SIMCONNECT_DATA_INITPOSITION initPos = {};
                            initPos.Altitude  = m_dataUserObject.dAlt;
                            initPos.Latitude  = m_dataUserObject.dLat;
                            initPos.Longitude = m_dataUserObject.dLon;
                            initPos.Heading   = (double)(((int)m_dataUserObject.dHead + 90) % 360);
                            initPos.OnGround  = 1;

                            // Move it 50 feet in front of user aircraft
                            Translate (m_dataUserObject.dHead, 50.0, initPos.Latitude, initPos.Longitude);

                            // Create the ground vehicle
                            SimConnect_AICreateSimulatedObject (
                                m_hSimConnect,
                                #ifdef SIM_MSFS2020
                                    "ASO_Pushback_Blue",
                                #else
                                    "VEH_jetTruck",
                                #endif
                                initPos,
                                DATA_REQ_ID_GROUND_VEHICLE
                            );
                        }
                        break;
                    }

                    case EVENT_ID_QUIT:
                        _tprintf (_T("QUIT key pressed.\n"));
                        m_bQuit = true;
                        break;

                    case EVENT_ID_RUDDER_LEFT:
                        if (!m_idObjGroundVehicle)
                        {
                            _tprintf (_T("Create the ground vehicle first!\n"));
                        }
                        else if (m_dataGroundVehicle.dRudderPos > -1.0)
                        {
                            m_dataGroundVehicle.dRudderPos -= 0.1;
                            _tprintf (_T("Setting rudder position to %f...\n"), m_dataGroundVehicle.dRudderPos);

                            SimConnect_SetDataOnSimObject (
                                m_hSimConnect,
                                DATA_DEF_ID_GROUND_VEHICLE,
                                m_idObjGroundVehicle,
                                SIMCONNECT_DATA_SET_FLAG_DEFAULT,
                                1,
                                sizeof (m_dataGroundVehicle),
                                &m_dataGroundVehicle
                            );
                        }
                        break;

                    case EVENT_ID_RUDDER_RIGHT:
                        if (!m_idObjGroundVehicle)
                        {
                            _tprintf (_T("Create the ground vehicle first!\n"));
                        }
                        else if (m_dataGroundVehicle.dRudderPos < 1.0)
                        {
                            m_dataGroundVehicle.dRudderPos += 0.1;
                            _tprintf (_T("Setting rudder position to %f...\n"), m_dataGroundVehicle.dRudderPos);

                            SimConnect_SetDataOnSimObject (
                                m_hSimConnect,
                                DATA_DEF_ID_GROUND_VEHICLE,
                                m_idObjGroundVehicle,
                                SIMCONNECT_DATA_SET_FLAG_DEFAULT,
                                1,
                                sizeof (m_dataGroundVehicle),
                                &m_dataGroundVehicle
                            );
                        }
                        break;
                }
                break;
            }

            case SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID:
            {
                SIMCONNECT_RECV_ASSIGNED_OBJECT_ID* pObjData = (SIMCONNECT_RECV_ASSIGNED_OBJECT_ID*)pData;

                switch (pObjData->dwRequestID)
                {
                    case DATA_REQ_ID_GROUND_VEHICLE:
                        m_idObjGroundVehicle = pObjData->dwObjectID;
                        _tprintf (_T("Recevied object id %u for ground vehicle.\n"), m_idObjGroundVehicle);

                        // Request data on ground vehicle
                        SimConnect_RequestDataOnSimObject (
                            m_hSimConnect,
                            DATA_REQ_ID_GROUND_VEHICLE,
                            DATA_DEF_ID_GROUND_VEHICLE,
                            m_idObjGroundVehicle,
                            SIMCONNECT_PERIOD_SIM_FRAME,
                            SIMCONNECT_DATA_REQUEST_FLAG_CHANGED
                        );
                        break;
                }
                break;
            }

            case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
            {
                SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;

                switch (pObjData->dwRequestID)
                {
                    case DATA_REQ_ID_GROUND_VEHICLE:
                        m_dataGroundVehicle = *((DataGroundVehicle*)&pObjData->dwData);
                        _tprintf (_T("Rudder position is now %f\n"), m_dataGroundVehicle.dRudderPos);
                        break;

                    case DATA_REQ_ID_USER_OBJECT:
                        m_dataUserObject     = *((DataUserObject*)&pObjData->dwData);
                        m_bDataUserObjectSet = true;

                        _tprintf (_T("Received data for user object: lat=%f, lon=%f, head=%f, alt=%f\n"),
                                  m_dataUserObject.dLat, m_dataUserObject.dLon, m_dataUserObject.dHead, m_dataUserObject.dAlt);
                        break;
                }
                break;
            }

            case SIMCONNECT_RECV_ID_QUIT:
                _tprintf (_T("Simulator quit received.\n"));
                m_bQuit = true;
                break;

            case SIMCONNECT_RECV_ID_EXCEPTION:
            {
                SIMCONNECT_RECV_EXCEPTION* pEx = (SIMCONNECT_RECV_EXCEPTION*)pData;
                _tprintf (_T("Exception! Code=%u, Message=%s\n"),
                          pEx->dwException, GetExceptionStr ((SIMCONNECT_EXCEPTION)pEx->dwException));
                break;
            }
        }
    }

    /**
     * Static method that calls the instance, which is passed as the context.
     */
    static void CALLBACK DispatchProc_ (SIMCONNECT_RECV* pData,
                                        DWORD            cbData,
                                        void*            pContext)
    {
        CDemoRudderPos* pThis = (CDemoRudderPos*)pContext;
        pThis->DispatchProc (pData, cbData);
    }

    /**
     * Move (translate) a specified distance in feet at a specified true heading in degrees.
     */
    static void Translate (double  dHeading,
                           double  dDistance,
                           double& dLatitude,
                           double& dLongitude)
    {
        if (dDistance == 0.0) return; // Nothing to do

        // Convert to radians
        dHeading   *= DEG_TO_RAD; // Also want the radial
        dDistance  *= FT_TO_RAD;
        dLatitude  *= DEG_TO_RAD;
        dLongitude *= -DEG_TO_RAD; // Negative, since this formula assumes positive for north, but FSX is the opposite

        dLatitude = asin (sin (dLatitude) * cos (dDistance) + cos (dLatitude) * sin (dDistance) * cos (dHeading));
        if (cos (dLatitude) != 0.0)
        {
            dLongitude = Mod (dLongitude - asin (sin (dHeading) * sin (dDistance) / cos (dLatitude)) + M_PI, M_PI * 2.0) - M_PI;
        }

        // Convert back to degrees
        dLatitude  *= RAD_TO_DEG;
        dLongitude *= -RAD_TO_DEG;
    }

    /**
     * Modulus (i.e. remainder) of a division. Unlike C-library fmod where the numerator sign is preserved, here the
     *  denominator sign is preserved.
     */
    static double Mod (double dNumer,
                       double dDenom)
    {
        return dNumer - dDenom * floor (dNumer / dDenom);
    }

    const TCHAR* GetExceptionStr (SIMCONNECT_EXCEPTION exception)
    {
        switch (exception)
        {
            case SIMCONNECT_EXCEPTION_NONE:                                 return _T("None");
            case SIMCONNECT_EXCEPTION_ERROR:                                return _T("Error");
            case SIMCONNECT_EXCEPTION_SIZE_MISMATCH:                        return _T("Size Mismatch");
            case SIMCONNECT_EXCEPTION_UNRECOGNIZED_ID:                      return _T("Unrecognized Id");
            case SIMCONNECT_EXCEPTION_UNOPENED:                             return _T("Unopened");
            case SIMCONNECT_EXCEPTION_VERSION_MISMATCH:                     return _T("Version Mismatch");
            case SIMCONNECT_EXCEPTION_TOO_MANY_GROUPS:                      return _T("Too Many Groups");
            case SIMCONNECT_EXCEPTION_NAME_UNRECOGNIZED:                    return _T("Name Unrecognized");
            case SIMCONNECT_EXCEPTION_TOO_MANY_EVENT_NAMES:                 return _T("Too Many Event Names");
            case SIMCONNECT_EXCEPTION_EVENT_ID_DUPLICATE:                   return _T("Event Id Duplicate");
            case SIMCONNECT_EXCEPTION_TOO_MANY_MAPS:                        return _T("Too Many Maps");
            case SIMCONNECT_EXCEPTION_TOO_MANY_OBJECTS:                     return _T("Too Many Objects");
            case SIMCONNECT_EXCEPTION_TOO_MANY_REQUESTS:                    return _T("Too Many Requests");
            case SIMCONNECT_EXCEPTION_WEATHER_INVALID_PORT:                 return _T("Weather Invalid Port");
            case SIMCONNECT_EXCEPTION_WEATHER_INVALID_METAR:                return _T("Weather Invalid Metar");
            case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_GET_OBSERVATION:    return _T("Weather Unable to Get Observation");
            case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_CREATE_STATION:     return _T("Weather Unable to Create Station");
            case SIMCONNECT_EXCEPTION_WEATHER_UNABLE_TO_REMOVE_STATION:     return _T("Weather Unable to Remove Station");
            case SIMCONNECT_EXCEPTION_INVALID_DATA_TYPE:                    return _T("Invalid Data Type");
            case SIMCONNECT_EXCEPTION_INVALID_DATA_SIZE:                    return _T("Invalid Data Size");
            case SIMCONNECT_EXCEPTION_DATA_ERROR:                           return _T("Data Error");
            case SIMCONNECT_EXCEPTION_INVALID_ARRAY:                        return _T("Invalid Array");
            case SIMCONNECT_EXCEPTION_CREATE_OBJECT_FAILED:                 return _T("Create Object Failed");
            case SIMCONNECT_EXCEPTION_LOAD_FLIGHTPLAN_FAILED:               return _T("Load Flightplan Failed");
            case SIMCONNECT_EXCEPTION_OPERATION_INVALID_FOR_OBJECT_TYPE:    return _T("Operation Invalid For Object Type");
            case SIMCONNECT_EXCEPTION_ILLEGAL_OPERATION:                    return _T("Illegal Operation");
            case SIMCONNECT_EXCEPTION_ALREADY_SUBSCRIBED:                   return _T("Already Subscribed");
            case SIMCONNECT_EXCEPTION_INVALID_ENUM:                         return _T("Invalid Enum");
            case SIMCONNECT_EXCEPTION_DEFINITION_ERROR:                     return _T("Definition Error");
            case SIMCONNECT_EXCEPTION_DUPLICATE_ID:                         return _T("Duplicate Id");
            case SIMCONNECT_EXCEPTION_DATUM_ID:                             return _T("Datum Id");
            case SIMCONNECT_EXCEPTION_OUT_OF_BOUNDS:                        return _T("Out of Bounds");
            case SIMCONNECT_EXCEPTION_ALREADY_CREATED:                      return _T("Already Created");
            case SIMCONNECT_EXCEPTION_OBJECT_OUTSIDE_REALITY_BUBBLE:        return _T("Object Outside Reality Bubble");
            case SIMCONNECT_EXCEPTION_OBJECT_CONTAINER:                     return _T("Object Container");
            case SIMCONNECT_EXCEPTION_OBJECT_AI:                            return _T("Object AI");
            case SIMCONNECT_EXCEPTION_OBJECT_ATC:                           return _T("Object ATC");
            case SIMCONNECT_EXCEPTION_OBJECT_SCHEDULE:                      return _T("Object Schedule");
            default:                                                        return _T("(Unknown)");
        }
    }


    HANDLE              m_hSimConnect;
    bool                m_bQuit;
    DWORD               m_idObjGroundVehicle;
    DataUserObject      m_dataUserObject;
    DataGroundVehicle   m_dataGroundVehicle;
    bool                m_bDataUserObjectSet;
};


int __cdecl _tmain (int argc, _TCHAR* argv[])
{
    CDemoRudderPos demo;
    demo.Run ();
    return 0;
}
