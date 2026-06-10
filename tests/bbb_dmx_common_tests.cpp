#include <array>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "bbb/dmx/common.hpp"
#include "bbb/dmx/movertrack.hpp"

namespace {

bool nearly_equal(double left, double right, double tolerance = 1.0e-9) {
    return std::abs(left - right) <= tolerance;
}

void require(bool condition, const char *message) {
    if(!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::exit(1);
    }
}

} // namespace

int main() {
    require(bbb::dmx::angle_to_u16(-270.0, 540.0) == 0, "pan range minimum maps to 0");
    require(bbb::dmx::angle_to_u16(270.0, 540.0) == 65535, "pan range maximum maps to 65535");
    require(bbb::dmx::angle_to_u16(0.0, 540.0) == 32768, "center uses round() and maps to 32768");

    const std::array<int, 4> coarse_fine{bbb::dmx::pan_tilt_to_bytes(0x1234, 0xABCD, bbb::dmx::byte_order::coarse_fine)};
    require(coarse_fine == std::array<int, 4>{18, 52, 171, 205}, "coarse/fine byte order");

    const std::array<int, 4> fine_coarse{bbb::dmx::pan_tilt_to_bytes(0x1234, 0xABCD, bbb::dmx::byte_order::fine_coarse)};
    require(fine_coarse == std::array<int, 4>{52, 18, 205, 171}, "fine/coarse byte order");

    require(nearly_equal(bbb::dmx::choose_shortest_pan(-179.0, 179.0), 181.0), "shortest pan crosses atan2 wrap");

    const bbb::dmx::pan_tilt_degrees forward{bbb::dmx::vector_to_pan_tilt(bbb::dmx::vec3{0.0, 10.0, 0.0})};
    require(nearly_equal(forward.pan, 0.0), "forward pan is zero");
    require(nearly_equal(forward.tilt, 0.0), "forward tilt is zero");

    const bbb::dmx::pan_tilt_degrees right{bbb::dmx::vector_to_pan_tilt(bbb::dmx::vec3{10.0, 0.0, 0.0})};
    require(nearly_equal(right.pan, 90.0), "right pan is +90");
    require(nearly_equal(right.tilt, 0.0), "right tilt is zero");

    const bbb::dmx::pan_tilt_degrees upward{bbb::dmx::vector_to_pan_tilt(bbb::dmx::vec3{0.0, 10.0, 10.0})};
    require(nearly_equal(upward.pan, 0.0), "upward pan is zero");
    require(nearly_equal(upward.tilt, 45.0), "upward tilt is +45");

    const bbb::dmx::vec3 world_forward{0.0, 10.0, 0.0};
    const bbb::dmx::vec3 local{bbb::dmx::world_to_fixture_local(world_forward, 0.0, 0.0, 90.0)};
    require(nearly_equal(local.x, 10.0), "inverse fixture rotation converts world forward to local right for rz=90");
    require(nearly_equal(local.y, 0.0, 1.0e-8), "inverse fixture rotation y component near zero");

    bbb::dmx::movertrack_engine engine{};
    auto output = engine.compute(bbb::dmx::vec3{0.0, 10.0, 0.0});
    require(nearly_equal(output.pan_degrees, 0.0), "movertrack forward pan degrees");
    require(nearly_equal(output.tilt_degrees, 0.0), "movertrack forward tilt degrees");
    require(output.pan == 32768, "movertrack forward pan value");
    require(output.tilt == 32768, "movertrack forward tilt value");

    output = engine.compute(bbb::dmx::vec3{10.0, 0.0, 0.0});
    require(nearly_equal(output.pan_degrees, 90.0), "movertrack right pan degrees");

    bbb::dmx::movertrack_engine tracking_engine{};
    output = tracking_engine.compute(bbb::dmx::vec3{0.0174524064, -0.999847695, 0.0});
    require(nearly_equal(output.pan_degrees, 179.0, 1.0e-6), "movertrack initializes near +179");
    output = tracking_engine.compute(bbb::dmx::vec3{-0.0174524064, -0.999847695, 0.0});
    require(nearly_equal(output.pan_degrees, 181.0, 1.0e-6), "movertrack tracks shortest pan to +181");

    bbb::dmx::movertrack_engine clamp_engine{};
    clamp_engine.set_ranges(180.0, 270.0);
    output = clamp_engine.compute(bbb::dmx::vec3{10.0, 0.0, 0.0});
    require(nearly_equal(output.pan_degrees, 90.0), "movertrack pan at clamp boundary");
    output = clamp_engine.compute(bbb::dmx::vec3{0.0174524064, -0.999847695, 0.0});
    require(nearly_equal(output.pan_degrees, 90.0), "movertrack clamps pan above range");

    bbb::dmx::byte_order parsed_order{bbb::dmx::byte_order::coarse_fine};
    require(bbb::dmx::byte_order_from_string("finecoarse", parsed_order), "parse finecoarse");
    require(parsed_order == bbb::dmx::byte_order::fine_coarse, "parsed finecoarse value");
    require(!bbb::dmx::byte_order_from_string("bad", parsed_order), "reject bad byte order");

    std::cout << "bbb_dmx_common_tests passed" << std::endl;
    return 0;
}
