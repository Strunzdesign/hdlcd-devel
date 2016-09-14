set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost REQUIRED COMPONENTS system)
INCLUDE_DIRECTORIES( ${Boost_INCLUDE_DIR} )

install(FILES
    Frame.h
    FrameEndpoint.h
    HdlcdClient.h
    HdlcdConfig.h
    HdlcdPacket.h
    HdlcdPacketCtrl.h
    HdlcdPacketData.h
    HdlcdPacketEndpoint.h
    HdlcdSessionDescriptor.h
    HdlcdSessionHeader.h
DESTINATION include)
