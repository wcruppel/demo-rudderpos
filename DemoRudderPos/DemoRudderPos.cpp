#include <Windows.h>
#include <SimConnect.h>
#include <comdef.h>
#include <tchar.h>

#pragma comment (lib, "SimConnect.lib")


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
                _T("  Q to quit\n")
            );

            // Create private events
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_CREATE);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_RUDDER_LEFT);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_RUDDER_RIGHT);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID_QUIT);

            // Link the private events to keyboard keys
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID_KEYBOARD, "C", EVENT_ID_CREATE);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID_KEYBOARD, "A", EVENT_ID_RUDDER_LEFT);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID_KEYBOARD, "D", EVENT_ID_RUDDER_RIGHT);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID_KEYBOARD, "Q", EVENT_ID_QUIT);

            // Ensure input events are off
            SimConnect_SetInputGroupState (m_hSimConnect, GROUP_ID_KEYBOARD, SIMCONNECT_STATE_OFF);

            // Sign up for notifications to input events
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID_KEYBOARD, EVENT_ID_CREATE);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID_KEYBOARD, EVENT_ID_RUDDER_LEFT);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID_KEYBOARD, EVENT_ID_RUDDER_RIGHT);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID_KEYBOARD, EVENT_ID_QUIT);

            // Set up the definition for the user object
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
                "PLANE ALTITUDE",
                "feet",
                SIMCONNECT_DATATYPE_FLOAT64
            );

            // Set up the definition for the ground vehicle
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID_GROUND_VEHICLE,
                "RUDDER POSITION",
                "position",
                SIMCONNECT_DATATYPE_FLOAT64
            );

            // Request a simulation start event
            SimConnect_SubscribeToSystemEvent (m_hSimConnect, EVENT_ID_RUDDER_SIM_START, "SimStart");

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
        EVENT_ID_RUDDER_SIM_START,
        EVENT_ID_CREATE,
        EVENT_ID_RUDDER_RIGHT,
        EVENT_ID_RUDDER_LEFT,
        EVENT_ID_QUIT
    };

    enum REQUEST_ID
    {
        REQUEST_ID_USER_OBJECT,
        REQUEST_ID_GROUND_VEHICLE
    };

    enum DATA_DEF_ID
    {
        DATA_DEF_ID_USER_OBJECT,
        DATA_DEF_ID_GROUND_VEHICLE
    };

    enum GROUP_ID
    {
        GROUP_ID_KEYBOARD
    };

    enum class INPUT_ID
    {
        Keyboard
    };

#pragma pack (push, 1)
    typedef struct DataUserObject
    {
        DataUserObject ()
        {
            dLat = 0.0;
            dLon = 0.0;
            dAlt = 0.0;
        }
        
        double dLat;
        double dLon;
        double dAlt;
    }
    DataUserObject;
#pragma pack (pop)

#pragma pack (push, 1)
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
                    case EVENT_ID_RUDDER_SIM_START:
                        // Sim has started so turn the input events on
                        SimConnect_SetInputGroupState (m_hSimConnect, GROUP_ID_KEYBOARD, SIMCONNECT_STATE_ON);
                        break;

                    case EVENT_ID_CREATE:
                    {
                        if (!m_bDataUserObjectSet)
                        {
                            _tprintf (_T("No data from user object yet!\n"));
                            break;
                        }

                        // TODO: Add heading to user obj and move GV in front 50 feet
                        SIMCONNECT_DATA_INITPOSITION initPos = {};
                        initPos.Altitude  = m_dataUserObject.dAlt;
                        initPos.Latitude  = m_dataUserObject.dLat;
                        initPos.Longitude = m_dataUserObject.dLon;
                        initPos.OnGround  = 1;

                        SimConnect_AICreateSimulatedObject (m_hSimConnect, "VEH_jetTruck", initPos, REQUEST_ID_GROUND_VEHICLE);
                        break;
                    }

                    case EVENT_ID_QUIT:
                        _tprintf (_T("QUIT key pressed.\n"));
                        m_bQuit = true;
                        break;
                }
            }

            case SIMCONNECT_RECV_ID_ASSIGNED_OBJECT_ID:
            {
                SIMCONNECT_RECV_ASSIGNED_OBJECT_ID* pObjData = (SIMCONNECT_RECV_ASSIGNED_OBJECT_ID*)pData;

                switch (pObjData->dwRequestID)
                {
                    case REQUEST_ID_GROUND_VEHICLE:
                        m_idObjGroundVehicle = pObjData->dwObjectID;
                        _tprintf (_T ("Recevied object id %u for ground vehicle.\n"), m_idObjGroundVehicle);
                        break;
                }
            }

            case SIMCONNECT_RECV_ID_SIMOBJECT_DATA:
            {
                SIMCONNECT_RECV_SIMOBJECT_DATA* pObjData = (SIMCONNECT_RECV_SIMOBJECT_DATA*)pData;

                switch (pObjData->dwRequestID)
                {
                    case REQUEST_ID_GROUND_VEHICLE:
                    {
                        DataGroundVehicle dataGV = *((DataGroundVehicle*)&pObjData->dwData);
                        _tprintf (_T("Rudder position is now: %f\n"), dataGV.dRudderPos);
                        break;
                    }

                    case REQUEST_ID_USER_OBJECT:
                    {
                        m_dataUserObject     = *((DataUserObject*)&pObjData->dwData);
                        m_bDataUserObjectSet = true;

                        _tprintf (_T("Received data for user object: lat=%f, lon=%f, alt=%f\n"),
                                  m_dataUserObject.dLat, m_dataUserObject.dLon, m_dataUserObject.dAlt);
                        break;
                    }
                }
                break;
            }

            case SIMCONNECT_RECV_ID_QUIT:
                _tprintf (_T("Simulator quit received.\n"));
                m_bQuit = true;
                break;
        }
    }

    static void CALLBACK DispatchProc_ (SIMCONNECT_RECV* pData,
                                        DWORD            cbData,
                                        void*            pContext)
    {
        CDemoRudderPos* pThis = (CDemoRudderPos*)pContext;
        pThis->DispatchProc (pData, cbData);
    }

    HANDLE          m_hSimConnect;
    bool            m_bQuit;
    DWORD           m_idObjGroundVehicle;
    DataUserObject  m_dataUserObject;
    bool            m_bDataUserObjectSet;
};


int __cdecl _tmain (int argc, _TCHAR* argv[])
{
    CDemoRudderPos demo;
    demo.Run ();
    return 0;
}
