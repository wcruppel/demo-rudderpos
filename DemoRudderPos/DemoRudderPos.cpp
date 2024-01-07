#include <SimConnect.h>
#include <comdef.h>
#include <tchar.h>


class CDemoRudderPos
{
public:
    CDemoRudderPos () :
        m_hSimConnect (NULL),
        m_bQuit       (false)
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
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID::Create);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID::RudderLeft);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID::RudderRight);
            SimConnect_MapClientEventToSimEvent (m_hSimConnect, EVENT_ID::Quit);

            // Link the private events to keyboard keys
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID::Keyboard, "C", EVENT_ID::Create);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID::Keyboard, "A", EVENT_ID::RudderLeft);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID::Keyboard, "D", EVENT_ID::RudderRight);
            SimConnect_MapInputEventToClientEvent (m_hSimConnect, GROUP_ID::Keyboard, "Q", EVENT_ID::Quit);

            // Ensure input events are off
            SimConnect_SetInputGroupState (m_hSimConnect, GROUP_ID::Keyboard, SIMCONNECT_STATE_OFF);

            // Sign up for notifications to input events
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID::Keyboard, EVENT_ID::Create);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID::Keyboard, EVENT_ID::RudderLeft);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID::Keyboard, EVENT_ID::RudderRight);
            SimConnect_AddClientEventToNotificationGroup (m_hSimConnect, GROUP_ID::Keyboard, EVENT_ID::Quit);

            // Set up the definition for the user object
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID::UserObject,
                "PLANE LATITUDE",
                "degrees",
                SIMCONNECT_DATATYPE_FLOAT64
            );
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID::UserObject,
                "PLANE LONGITUDE",
                "degrees",
                SIMCONNECT_DATATYPE_FLOAT64
            );
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID::UserObject,
                "PLANE ALTITUDE",
                "feet",
                SIMCONNECT_DATATYPE_FLOAT64
            );

            // Set up the definition for the ground vehicle
            SimConnect_AddToDataDefinition (
                m_hSimConnect,
                DATA_DEF_ID::GroundVehicle,
                "RUDDER POSITION",
                "position",
                SIMCONNECT_DATATYPE_FLOAT64
            );

            // Request a simulation start event
            SimConnect_SubscribeToSystemEvent (m_hSimConnect, EVENT_ID::SimStart, "SimStart");

            while (!m_bQuit)
            {
                SimConnect_CallDispatch (m_hSimConnect, DispatchProc, this);
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
    enum class EVENT_ID : int
    {
        SimStart,
        Create,
        RudderRight,
        RudderLeft,
        Quit
    };

    enum class REQUEST_ID : int
    {
        UserObject,
        GroundVehicle
    };

    enum class DATA_DEF_ID : int
    {
        UserObject,
        GroundVehicle
    };

    enum class GROUP_ID : int
    {
        Keyboard
    };

    enum class INPUT_ID : int
    {
        Keyboard
    };

#pragma pack(1)
    struct DataUserObject
    {
        double dLat;
        double dLon;
        double dAlt;
    };
#pragma unpack


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
                    case EVENT_ID::SimStart:
                        // Sim has started so turn the input events on
                        SimConnect_SetInputGroupState (m_hSimConnect, GROUP_ID::Keyboard, SIMCONNECT_STATE_ON);
                        break;

                    case EVENT_ID::Create:
                    {
                        // KSEA
                        SIMCONNECT_DATA_INITPOSITION initPos = {};
                        initPos.Altitude  = 433.0;
                        initPos.Latitude  = 47 + (25.91 / 60);
                        initPos.Longitude = -122 - (18.47 / 60);
                        initPos.Heading   = 360.0;
                        initPos.OnGround  = 1;

                        SimConnect_AICreateSimulatedObject (m_hSimConnect, "VEH_jetTruck", initPos, REQUEST_ID::GroundVehicle);
                        break;
                    }

                    case EVENT_ID::Quit:
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
                    case REQUEST_ID::GroundVehicle:
                        m_idObjGroundVehicle = pObjData->dwObjectID;
                        _tprintf (_T ("Recevied object id %u for ground vehicle.\n"), m_idObjGroundVehicle);
                        break;
                }
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

    HANDLE  m_hSimConnect;
    bool    m_bQuit;
    DWORD   m_idObjGroundVehicle;
};


int __cdecl _tmain (int argc, _TCHAR* argv[])
{
    CDemoRudderPos demo;
    demo.Run ();
    return 0;
}
