#include "files.h"
#include "gpu/decl_md.h"
#include "test/ff.h"
#include "test/rt.h"
#include "test/test.h"

m_tinker_using_namespace;
using namespace test;

const char* verlet_intg = "integrator  verlet\n";
static int usage_ = gpu::use_xyz | gpu::use_vel | gpu::use_mass |
    gpu::use_energy | gpu::use_grad;

TEST_CASE("Verlet-Ar6", "[noassert][verlet][ar6]") {
  const char* k = "test_ar6.key";
  const char* d = "test_ar6.dyn";
  const char* x1 = "test_ar6.xyz";
  const char* p = "amoeba09.prm";

  std::string k0 = ar6_key;
  k0 += verlet_intg;
  file fke(k, k0);

  file fd(d, ar6_dyn);
  file fx1(x1, ar6_xyz);
  file fpr(p, amoeba09_prm);

  const char* argv[] = {"dummy", x1};
  int argc = 2;
  test_begin_1_xyz(argc, argv);
  test_mdinit(0, 0);

  gpu::use_data = usage_;
  tinker_gpu_data_create();

  gpu::md_data(static_cast<gpu::rc_t>(gpu::rc_alloc | gpu::rc_copyin));
  gpu::velocity_verlet(1, 0.001);

  tinker_gpu_data_destroy();
  test_end();
}

static double arbox_kin[] = {
    64.648662, 64.666678, 64.684785, 64.702988, 64.721287, 64.739686, 64.758188,
    64.776794, 64.795508, 64.814330, 64.833263, 64.852309, 64.871469, 64.890744,
    64.910135, 64.929643, 64.949268, 64.969009, 64.988865, 65.008837, 65.028921,
    65.049117, 65.069421, 65.089830, 65.110341, 65.130949, 65.151650, 65.172437,
    65.193306, 65.214250, 65.235261, 65.256333, 65.277457, 65.298624, 65.319825,
    65.341051, 65.362293, 65.383538, 65.404778, 65.426001, 65.447194, 65.468348,
    65.489449, 65.510487, 65.531448, 65.552320, 65.573090, 65.593747, 65.614277,
    65.634667, 65.654906, 65.674979, 65.694876, 65.714582, 65.734087, 65.753378,
    65.772443, 65.791271, 65.809851, 65.828172, 65.846223, 65.863994, 65.881475,
    65.898657, 65.915530, 65.932087, 65.948319, 65.964217, 65.979776, 65.994987,
    66.009845, 66.024343, 66.038477, 66.052239, 66.065627, 66.078635, 66.091259,
    66.103495, 66.115340, 66.126791, 66.137844, 66.148498, 66.158749, 66.168596,
    66.178037, 66.187070, 66.195692, 66.203903, 66.211700, 66.219083, 66.226048,
    66.232596, 66.238723, 66.244429, 66.249712, 66.254569, 66.258999, 66.263000,
    66.266569, 66.269704};
static double arbox_pot[] = {
    -277.037089, -277.055104, -277.073212, -277.091414, -277.109713,
    -277.128112, -277.146613, -277.165219, -277.183932, -277.202753,
    -277.221686, -277.240732, -277.259892, -277.279167, -277.298557,
    -277.318065, -277.337689, -277.357430, -277.377286, -277.397258,
    -277.417342, -277.437538, -277.457842, -277.478251, -277.498762,
    -277.519370, -277.540071, -277.560859, -277.581728, -277.602672,
    -277.623683, -277.644755, -277.665879, -277.687047, -277.708249,
    -277.729476, -277.750717, -277.771964, -277.793204, -277.814427,
    -277.835621, -277.856776, -277.877878, -277.898916, -277.919877,
    -277.940750, -277.961521, -277.982179, -278.002709, -278.023100,
    -278.043339, -278.063414, -278.083311, -278.103018, -278.122523,
    -278.141815, -278.160881, -278.179710, -278.198290, -278.216611,
    -278.234663, -278.252434, -278.269916, -278.287099, -278.303973,
    -278.320530, -278.336762, -278.352661, -278.368219, -278.383431,
    -278.398289, -278.412788, -278.426922, -278.440685, -278.454073,
    -278.467081, -278.479705, -278.491941, -278.503786, -278.515237,
    -278.526291, -278.536944, -278.547196, -278.557043, -278.566484,
    -278.575517, -278.584139, -278.592350, -278.600148, -278.607530,
    -278.614496, -278.621043, -278.627171, -278.632877, -278.638159,
    -278.643017, -278.647447, -278.651448, -278.655017, -278.658152};
TEST_CASE("NVE-Verlet-ArBox", "[ff][nve][verlet][arbox]") {
#ifdef TINKER_GPU_DOUBLE
  const char* k = "test_arbox.key";
  const char* d = "test_arbox.dyn";
  const char* x = "test_arbox.xyz";
  const char* p = "amoeba09.prm";

  std::string k0 = arbox_key;
  k0 += "integrator  verlet\n";
  file fke(k, k0);

  file fd(d, arbox_dyn);
  file fx(x, arbox_xyz);
  file fp(p, amoeba09_prm);

  const char* argv[] = {"dummy", x};
  int argc = 2;
  test_begin_1_xyz(argc, argv);
  test_mdinit(0, 0);

  gpu::use_data = usage_;
  tinker_gpu_data_create();

  gpu::md_data(static_cast<gpu::rc_t>(gpu::rc_alloc | gpu::rc_copyin));
  const double dt_ps = 0.001;
  const int nsteps = 20;
  const double eps_e = 0.0001;
  std::vector<double> epots, eksums;
  for (int i = 1; i <= nsteps; ++i) {
    gpu::velocity_verlet(i, dt_ps);
    epots.push_back(gpu::get_energy(gpu::esum));
    eksums.push_back(gpu::eksum);
  }

  tinker_gpu_data_destroy();
  test_end();

  for (int i = 0; i < nsteps; ++i) {
    REQUIRE(epots[i] == Approx(arbox_pot[i]).margin(eps_e));
    REQUIRE(eksums[i] == Approx(arbox_kin[i]).margin(eps_e));
  }

  file_expected("test_arbox.arc");
#else
  REQUIRE(true);
#endif
}
