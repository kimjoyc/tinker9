#include "add.h"
#include "empole_chgpen.h"
#include "epolar_chgpen.h"
#include "epolar.h"
#include "glob.nblist.h"
#include "image.h"
#include "md.h"
#include "pmestuf.h"
#include "potent.h"
#include "seq_pair_field_chgpen.h"
#include "switch.h"
#include "tool/gpu_card.h"

namespace tinker {
// see also subroutine udirect1 in induce.f
void dfield_chgpen_ewald_recip_self_acc(real (*field)[3])
{
   darray::zero(PROCEED_NEW_Q, n, field);

   const PMEUnit pu = ppme_unit;
   const real aewald = pu->aewald;
   const real term = aewald * aewald * aewald * 4 / 3 / sqrtpi;

   cmp_to_fmp(pu, cmp, fmp);
   grid_mpole(pu, fmp);
   fftfront(pu);
   if (vir_m)
      pme_conv(pu, vir_m);
   else
      pme_conv(pu);
   fftback(pu);
   fphi_mpole(pu);
   fphi_to_cphi(pu, fphi, cphi);

   #pragma acc parallel loop independent async deviceptr(field,cphi,rpole)
   for (int i = 0; i < n; ++i) {
      real dix = rpole[i][mpl_pme_x];
      real diy = rpole[i][mpl_pme_y];
      real diz = rpole[i][mpl_pme_z];
      field[i][0] += (-cphi[i][1] + term * dix);
      field[i][1] += (-cphi[i][2] + term * diy);
      field[i][2] += (-cphi[i][3] + term * diz);
   }
}

// see also subroutine udirect2b / dfield_chgpen0c in induce.f
#define DFIELD_DPTRS x, y, z, pcore, pval, palpha, field, rpole
void dfield_chgpen_ewald_real_acc(real (*field)[3])
{
   const real off = switch_off(switch_ewald);
   const real off2 = off * off;
   const int maxnlst = mlist_unit->maxnlst;
   const auto* mlst = mlist_unit.deviceptr();

   const PMEUnit pu = ppme_unit;
   const real aewald = pu->aewald;

   MAYBE_UNUSED int GRID_DIM = get_grid_size(BLOCK_DIM);
   #pragma acc parallel async num_gangs(GRID_DIM) vector_length(BLOCK_DIM)\
               deviceptr(DFIELD_DPTRS,mlst)
   #pragma acc loop gang independent
   for (int i = 0; i < n; ++i) {
      real xi = x[i];
      real yi = y[i];
      real zi = z[i];
      real ci = rpole[i][mpl_pme_0];
      real dix = rpole[i][mpl_pme_x];
      real diy = rpole[i][mpl_pme_y];
      real diz = rpole[i][mpl_pme_z];
      real qixx = rpole[i][mpl_pme_xx];
      real qixy = rpole[i][mpl_pme_xy];
      real qixz = rpole[i][mpl_pme_xz];
      real qiyy = rpole[i][mpl_pme_yy];
      real qiyz = rpole[i][mpl_pme_yz];
      real qizz = rpole[i][mpl_pme_zz];
      real corei = pcore[i];
      real alphai = palpha[i];
      real vali = pval[i];
      real gxi = 0, gyi = 0, gzi = 0;

      int nmlsti = mlst->nlst[i];
      int base = i * maxnlst;
      #pragma acc loop vector independent reduction(+:gxi,gyi,gzi)
      for (int kk = 0; kk < nmlsti; ++kk) {
         int k = mlst->lst[base + kk];
         real xr = x[k] - xi;
         real yr = y[k] - yi;
         real zr = z[k] - zi;

         real r2 = image2(xr, yr, zr);
         if (r2 <= off2) {
            real3 fid = make_real3(0, 0, 0);
            real3 fkd = make_real3(0, 0, 0);
            pair_dfield_chgpen<EWALD>( //
               r2, xr, yr, zr, 1, ci, dix, diy, diz, corei, vali, alphai,
               qixx, qixy, qixz, qiyy, qiyz, qizz, rpole[k][mpl_pme_0],
               rpole[k][mpl_pme_x], rpole[k][mpl_pme_y], rpole[k][mpl_pme_z],
               pcore[k], pval[k], palpha[k], rpole[k][mpl_pme_xx],
               rpole[k][mpl_pme_xy], rpole[k][mpl_pme_xz], rpole[k][mpl_pme_yy],
               rpole[k][mpl_pme_yz], rpole[k][mpl_pme_zz], aewald, fid, fkd);


            gxi += fid.x;
            gyi += fid.y;
            gzi += fid.z;


            atomic_add(fkd.x, &field[k][0]);
            atomic_add(fkd.y, &field[k][1]);
            atomic_add(fkd.z, &field[k][2]);
         }
      } // end for (int kk)

      atomic_add(gxi, &field[i][0]);
      atomic_add(gyi, &field[i][1]);
      atomic_add(gzi, &field[i][2]);
   } // end for (int i)

   #pragma acc parallel async\
               deviceptr(DFIELD_DPTRS,dexclude,dexclude_scale)
   #pragma acc loop independent
   for (int ii = 0; ii < ndexclude; ++ii) {
      int i = dexclude[ii][0];
      int k = dexclude[ii][1];
      real dscale = dexclude_scale[ii];

      real xi = x[i];
      real yi = y[i];
      real zi = z[i];
      real ci = rpole[i][mpl_pme_0];
      real dix = rpole[i][mpl_pme_x];
      real diy = rpole[i][mpl_pme_y];
      real diz = rpole[i][mpl_pme_z];
      real qixx = rpole[i][mpl_pme_xx];
      real qixy = rpole[i][mpl_pme_xy];
      real qixz = rpole[i][mpl_pme_xz];
      real qiyy = rpole[i][mpl_pme_yy];
      real qiyz = rpole[i][mpl_pme_yz];
      real qizz = rpole[i][mpl_pme_zz];
      real corei = pcore[i];
      real alphai = palpha[i];
      real vali = pval[i];

      real xr = x[k] - xi;
      real yr = y[k] - yi;
      real zr = z[k] - zi;

      real r2 = image2(xr, yr, zr);
      if (r2 <= off2) {
         real3 fid = make_real3(0, 0, 0);
         real3 fkd = make_real3(0, 0, 0);
         pair_dfield_chgpen<NON_EWALD>(
            r2, xr, yr, zr, dscale, ci, dix, diy, diz, corei, vali, alphai,
            qixx, qixy, qixz, qiyy, qiyz, qizz, rpole[k][mpl_pme_0],
            rpole[k][mpl_pme_x], rpole[k][mpl_pme_y], rpole[k][mpl_pme_z],
            pcore[k], pval[k], palpha[k], rpole[k][mpl_pme_xx],
            rpole[k][mpl_pme_xy], rpole[k][mpl_pme_xz], rpole[k][mpl_pme_yy],
            rpole[k][mpl_pme_yz], rpole[k][mpl_pme_zz], 0, fid, fkd);

         atomic_add(fid.x, &field[i][0]);
         atomic_add(fid.y, &field[i][1]);
         atomic_add(fid.z, &field[i][2]);

         atomic_add(fkd.x, &field[k][0]);
         atomic_add(fkd.y, &field[k][1]);
         atomic_add(fkd.z, &field[k][2]);
      }
   }
}

// see also subroutine umutual1 in induce.f
void ufield_chgpen_ewald_recip_self_acc(const real (*uind)[3], real (*field)[3])
{
   darray::zero(PROCEED_NEW_Q, n, field);

   const PMEUnit pu = ppme_unit;
   const auto& st = *pu;
   const int nfft1 = st.nfft1;
   const int nfft2 = st.nfft2;
   const int nfft3 = st.nfft3;
   const real aewald = st.aewald;

   cuind_to_fuind(pu, uind, uind, fuind, fuind);
   grid_uind(pu, fuind, fuind);
   fftfront(pu);
   // TODO: store vs. recompute qfac
   pme_conv(pu);
   fftback(pu);
   fphi_uind2(pu, fdip_phi1, fdip_phi2);

   const real term = aewald * aewald * aewald * 4 / 3 / sqrtpi;

   #pragma acc parallel loop independent async\
               deviceptr(field,uind,fdip_phi1)
   for (int i = 0; i < n; ++i) {
      real a[3][3];
      a[0][0] = nfft1 * recipa.x;
      a[1][0] = nfft2 * recipb.x;
      a[2][0] = nfft3 * recipc.x;
      a[0][1] = nfft1 * recipa.y;
      a[1][1] = nfft2 * recipb.y;
      a[2][1] = nfft3 * recipc.y;
      a[0][2] = nfft1 * recipa.z;
      a[1][2] = nfft2 * recipb.z;
      a[2][2] = nfft3 * recipc.z;

      #pragma acc loop seq
      for (int j = 0; j < 3; ++j) {
         real df1 = a[0][j] * fdip_phi1[i][1] + a[1][j] * fdip_phi1[i][2] +
            a[2][j] * fdip_phi1[i][3];
         field[i][j] += (term * uind[i][j] - df1);
      }
   }
}

#define UFIELD_DPTRS x, y, z, pcore, pval, palpha, field, uind
void ufield_chgpen_ewald_real_acc(const real (*uind)[3], real (*field)[3])

{
   const real off = switch_off(switch_ewald);
   const real off2 = off * off;
   const int maxnlst = mlist_unit->maxnlst;
   const auto* mlst = mlist_unit.deviceptr();

   const PMEUnit pu = ppme_unit;
   const real aewald = pu->aewald;

   MAYBE_UNUSED int GRID_DIM = get_grid_size(BLOCK_DIM);
   #pragma acc parallel async num_gangs(GRID_DIM) vector_length(BLOCK_DIM)\
               deviceptr(UFIELD_DPTRS,mlst)
   #pragma acc loop gang independent
   for (int i = 0; i < n; ++i) {
      real xi = x[i];
      real yi = y[i];
      real zi = z[i];
      real uindi0 = uind[i][0];
      real uindi1 = uind[i][1];
      real uindi2 = uind[i][2];
      real corei = pcore[i];
      real alphai = palpha[i];
      real vali = pval[i];
      real gxi = 0, gyi = 0, gzi = 0;

      int nmlsti = mlst->nlst[i];
      int base = i * maxnlst;
      #pragma acc loop vector independent reduction(+:gxi,gyi,gzi)
      for (int kk = 0; kk < nmlsti; ++kk) {
         int k = mlst->lst[base + kk];
         real xr = x[k] - xi;
         real yr = y[k] - yi;
         real zr = z[k] - zi;

         real r2 = image2(xr, yr, zr);
         if (r2 <= off2) {
            real3 fid = make_real3(0, 0, 0);
            real3 fkd = make_real3(0, 0, 0);
            pair_ufield_chgpen<EWALD>(r2, xr, yr, zr, 1, uindi0, uindi1, uindi2,
                                      corei, vali, alphai, uind[k][0],
                                      uind[k][1], uind[k][2], pcore[k], pval[k],
                                      palpha[k], aewald, fid, fkd);

            gxi += fid.x;
            gyi += fid.y;
            gzi += fid.z;

            atomic_add(fkd.x, &field[k][0]);
            atomic_add(fkd.y, &field[k][1]);
            atomic_add(fkd.z, &field[k][2]);
         }
      } // end for (int kk)

      atomic_add(gxi, &field[i][0]);
      atomic_add(gyi, &field[i][1]);
      atomic_add(gzi, &field[i][2]);

   } // end for (int i)

   #pragma acc parallel async\
               deviceptr(UFIELD_DPTRS,wexclude,wexclude_scale)
   #pragma acc loop independent
   for (int ii = 0; ii < nwexclude; ++ii) {
      int i = wexclude[ii][0];
      int k = wexclude[ii][1];
      real wscale = wexclude_scale[ii];

      real xi = x[i];
      real yi = y[i];
      real zi = z[i];
      real uindi0 = uind[i][0];
      real uindi1 = uind[i][1];
      real uindi2 = uind[i][2];
      real corei = pcore[i];
      real alphai = palpha[i];
      real vali = pval[i];

      real xr = x[k] - xi;
      real yr = y[k] - yi;
      real zr = z[k] - zi;

      real r2 = image2(xr, yr, zr);
      if (r2 <= off2) {
         real3 fid = make_real3(0, 0, 0);
         real3 fkd = make_real3(0, 0, 0);
         pair_ufield_chgpen<NON_EWALD>(r2, xr, yr, zr, wscale, uindi0, uindi1,
                                       uindi2, corei, vali, alphai, uind[k][0],
                                       uind[k][1], uind[k][2], pcore[k],
                                       pval[k], palpha[k], 0, fid, fkd);

         atomic_add(fid.x, &field[i][0]);
         atomic_add(fid.y, &field[i][1]);
         atomic_add(fid.z, &field[i][2]);

         atomic_add(fkd.x, &field[k][0]);
         atomic_add(fkd.y, &field[k][1]);
         atomic_add(fkd.z, &field[k][2]);
      }
   }
}
}
