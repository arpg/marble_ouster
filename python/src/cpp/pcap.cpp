/**
 * @file
 * @brief ouster_pyclient_pcap python module
 */
#include <pybind11/chrono.h>
#include <pybind11/eval.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

#include <csignal>
#include <cstdlib>
#include <string>
#include <thread>

#include "ouster/os_pcap.h"
// Disabled until buffers are replaced
//#include "ouster/ouster_pybuffer.h"
#include <pcap/pcap.h>

#include <sstream>

namespace py = pybind11;

/// @TODO The pybind11 buffer_info struct is actually pretty heavy-weight – it
/// includes some string fields, vectors, etc. Might be worth it to avoid
/// calling this twice.
/*
 * Check that buffer is a 1-d byte array of size > bound and return an internal
 * pointer to the data for writing. Check is strictly greater to account for the
 * extra byte required to determine if a datagram is bigger than expected.
 */
inline uint8_t* getptr(py::buffer& buf) {
    auto info = buf.request();
    if (info.format != py::format_descriptor<uint8_t>::format()) {
        throw std::invalid_argument(
            "Incompatible argument: expected a bytearray");
    }
    return (uint8_t*)info.ptr;
}
/// @TODO The pybind11 buffer_info struct is actually pretty heavy-weight – it
/// includes some string fields, vectors, etc. Might be worth it to avoid
/// calling this twice.
/*
 * Return the size of the python buffer
 */
inline size_t getptrsize(py::buffer& buf) {
    auto info = buf.request();

    return (size_t)info.size;
}

// hack: Wrap replay to allow terminating via SIGINT
// @TODO This is currently not thread safe
int replay_pcap(ouster::sensor_utils::playback_handle& handle, double rate) {
    py::gil_scoped_release release;

    auto py_handler = std::signal(SIGINT, [](int) { std::exit(EXIT_SUCCESS); });

    auto res = ouster::sensor_utils::replay(handle, rate);
    std::signal(SIGINT, py_handler);

    return res;
}

// Record functionality removed for a short amount of time
// until we switch it over to support libtins

PYBIND11_PLUGIN(_pcap) {
    py::module m ("_pcap", R"(Pcap bindings generated by pybind11.

This module is generated directly from the C++ code and not meant to be used
directly.
)");

    // turn off signatures in docstrings: mypy stubs provide better types
    py::options options;
    options.disable_function_signatures();

    // clang-format off
    
    py::class_<ouster::sensor_utils::stream_info,
               std::shared_ptr<ouster::sensor_utils::stream_info>>(m, "stream_info")
        .def(py::init<>())
        .def("__repr__", [](const ouster::sensor_utils::stream_info& data) {
                             std::stringstream result;
                             result << data;
                             return result.str();})
        .def_readonly("packets_processed", &ouster::sensor_utils::stream_info::packets_processed)
        .def_readonly("packets_reassembled", &ouster::sensor_utils::stream_info::packets_reassembled)
        .def_readonly("port_to_packet_sizes", &ouster::sensor_utils::stream_info::port_to_packet_sizes)
        .def_readonly("port_to_packet_count", &ouster::sensor_utils::stream_info::port_to_packet_count)
        .def_readonly("ipv6_packets", &ouster::sensor_utils::stream_info::ipv6_packets)
        .def_readonly("ipv4_packets", &ouster::sensor_utils::stream_info::ipv4_packets)
        .def_readonly("non_udp_packets", &ouster::sensor_utils::stream_info::non_udp_packets)
        .def_readonly("packet_size_to_port", &ouster::sensor_utils::stream_info::packet_size_to_port);
    
    py::class_<ouster::sensor_utils::packet_info,
               std::shared_ptr<ouster::sensor_utils::packet_info>>(m, "packet_info")
        .def(py::init<>())
        .def("__repr__", [](const ouster::sensor_utils::packet_info& data) {
                             std::stringstream result;
                             result << data;
                             return result.str();})
        .def_readonly("dst_ip", &ouster::sensor_utils::packet_info::dst_ip)
        .def_readonly("src_ip", &ouster::sensor_utils::packet_info::src_ip)
        .def_readonly("dst_port", &ouster::sensor_utils::packet_info::dst_port)
        .def_readonly("src_port", &ouster::sensor_utils::packet_info::src_port)
        .def_readonly("timestamp", &ouster::sensor_utils::packet_info::timestamp)
        .def_readonly("payload_size", &ouster::sensor_utils::packet_info::payload_size);
    
    
    py::class_<ouster::sensor_utils::playback_handle,
               std::shared_ptr<ouster::sensor_utils::playback_handle>>(m, "playback_handle")
        .def(py::init<>());
    py::class_<ouster::sensor_utils::record_handle,
               std::shared_ptr<ouster::sensor_utils::record_handle>>(m, "record_handle")
        .def(py::init<>());
    
    m.def("replay_pcap", &replay_pcap);
    m.def("replay_get_pcap_info", &ouster::sensor_utils::replay_get_pcap_info, py::return_value_policy::reference);

    m.def("replay_initialize",
          py::overload_cast<const std::string&, const std::string&,
          const std::string&, std::unordered_map<int, int>>(&ouster::sensor_utils::replay_initialize));
    m.def("replay_uninitialize", &ouster::sensor_utils::replay_uninitialize);
    m.def("replay_reset", &ouster::sensor_utils::replay_reset);
    m.def("replay_packet", &ouster::sensor_utils::replay_packet);

    m.def("next_packet_info", &ouster::sensor_utils::next_packet_info);
    m.def("read_packet",
          [](ouster::sensor_utils::playback_handle& handle, py::buffer buf) {
              return ouster::sensor_utils::read_packet(handle, getptr(buf), getptrsize(buf));
          });
    
    m.def("record_initialize", &ouster::sensor_utils::record_initialize);
    m.def("record_uninitialize", &ouster::sensor_utils::record_uninitialize);
    m.def("record_packet",
          [](ouster::sensor_utils::record_handle& handle,
             int src_port, int dst_port, py::buffer buf) {
              ouster::sensor_utils::record_packet(handle,
                                                  src_port,
                                                  dst_port,
                                                  getptr(buf),
                                                  getptrsize(buf));
          });
    // clang-format on

    return m.ptr();
}