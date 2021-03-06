﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;

namespace principia {
namespace ksp_plugin_adapter {

class BurnEditor : ScalingRenderer {
  public BurnEditor(PrincipiaPluginAdapter adapter,
                    Vessel vessel,
                    double initial_time,
                    int index,
                    BurnEditor previous_burn) {
    adapter_ = adapter;
    vessel_ = vessel;
    index_ = index;
    previous_burn_ = previous_burn;
    Δv_tangent_ =
        new DifferentialSlider(label            : "Δv tangent",
                               unit             : "m / s",
                               log10_lower_rate : Log10ΔvLowerRate,
                               log10_upper_rate : Log10ΔvUpperRate,
                               text_colour      : Style.Tangent);
    Δv_normal_ =
        new DifferentialSlider(label            : "Δv normal",
                               unit             : "m / s",
                               log10_lower_rate : Log10ΔvLowerRate,
                               log10_upper_rate : Log10ΔvUpperRate,
                               text_colour      : Style.Normal);
    Δv_binormal_ =
        new DifferentialSlider(label            : "Δv binormal",
                               unit             : "m / s",
                               log10_lower_rate : Log10ΔvLowerRate,
                               log10_upper_rate : Log10ΔvUpperRate,
                               text_colour      : Style.Binormal);
    previous_coast_duration_ =
        new DifferentialSlider(
                label            : "t initial",
                unit             : null,
                log10_lower_rate : Log10TimeLowerRate,
                log10_upper_rate : Log10TimeUpperRate,
                // We cannot have a coast of length 0, so let's make it very
                // short: that will be indistinguishable.
                zero_value       : 0.001,
                min_value        : 0,
                formatter        : FormatPreviousCoastDuration,
                parser           : TryParsePreviousCoastDuration);
    previous_coast_duration_.value = initial_time - time_base;
    reference_frame_selector_ = new ReferenceFrameSelector(
                                    adapter_,
                                    ReferenceFrameChanged,
                                    "Manœuvring frame");
    reference_frame_selector_.SetFrameParameters(
        adapter_.plotting_frame_selector_.FrameParameters());
    ComputeEngineCharacteristics();
  }

  // Renders the |BurnEditor|.  Returns true if and only if the settings were
  // changed.
  public bool Render(string header,
                     bool enabled,
                     double? actual_final_time = null) {
    bool changed = false;
    previous_coast_duration_.max_value =
        (actual_final_time ?? double.PositiveInfinity) - time_base;
    using (new UnityEngine.GUILayout.HorizontalScope()) {
      UnityEngine.GUILayout.Label(header);
      string frame_info = "";
      if (!reference_frame_selector_.FrameParameters().Equals(
              adapter_.plotting_frame_selector_.FrameParameters())) {
        frame_info = "Manœuvre frame differs from plotting frame";
      }
      UnityEngine.GUILayout.Label(
          frame_info,
          Style.RightAligned(Style.Info(UnityEngine.GUI.skin.label)));
    }
    using (new UnityEngine.GUILayout.VerticalScope()) {
      // When we are first rendered, the |initial_mass_in_tonnes_| will just have
      // been set.  If we have fallen back to instant impulse, we should use this
      // mass to set the thrust.
      if (first_time_rendering) {
        first_time_rendering = false;
        changed = true;
        engine_warning_ = "";
        ComputeEngineCharacteristics();
      }
      if (enabled) {
        using (new UnityEngine.GUILayout.HorizontalScope()) {
          if (UnityEngine.GUILayout.Button("Active Engines")) {
            engine_warning_ = "";
            ComputeEngineCharacteristics();
            changed = true;
          } else if (UnityEngine.GUILayout.Button("Active RCS")) {
            engine_warning_ = "";
            ComputeRCSCharacteristics();
            changed = true;
          } else if (UnityEngine.GUILayout.Button("Instant Impulse")) {
            engine_warning_ = "";
            UseTheForceLuke();
            changed = true;
          }
        }
        reference_frame_selector_.RenderButton();
      } else {
        reference_frame_selector_.Hide();
      }
      if (is_inertially_fixed_ !=
          UnityEngine.GUILayout.Toggle(is_inertially_fixed_,
                                       "Inertially fixed")) {
        changed = true;
        is_inertially_fixed_ = !is_inertially_fixed_;
      }
      changed |= Δv_tangent_.Render(enabled);
      changed |= Δv_normal_.Render(enabled);
      changed |= Δv_binormal_.Render(enabled);
      changed |= previous_coast_duration_.Render(enabled);
      UnityEngine.GUILayout.Label(
          index_ == 0 ? "Time base: start of flight plan"
                      : $"Time base: end of manœuvre #{index_}",
          style : new UnityEngine.GUIStyle(UnityEngine.GUI.skin.label){
              alignment = UnityEngine.TextAnchor.UpperLeft});
      changed |= changed_reference_frame_;
      using (new UnityEngine.GUILayout.HorizontalScope()) {
        UnityEngine.GUILayout.Label(
            "Manœuvre Δv : " + Δv().ToString("0.000") + " m/s",
            GUILayoutWidth(8));
        UnityEngine.GUILayout.Label("Duration : " + duration_.ToString("0.0") +
                                    " s");
      }
      UnityEngine.GUILayout.Label(engine_warning_,
                                  Style.Warning(UnityEngine.GUI.skin.label));
      changed_reference_frame_ = false;
    }
    return changed && enabled;
  }

  public double Δv() {
    return new Vector3d{x = Δv_tangent_.value,
                        y = Δv_normal_.value,
                        z = Δv_binormal_.value}.magnitude;
  }

  public void Reset(NavigationManoeuvre manoeuvre) {
    Burn burn = manoeuvre.burn;
    Δv_tangent_.value = burn.delta_v.x;
    Δv_normal_.value = burn.delta_v.y;
    Δv_binormal_.value = burn.delta_v.z;
    previous_coast_duration_.value =
        burn.initial_time - time_base;
    reference_frame_selector_.SetFrameParameters(burn.frame);
    is_inertially_fixed_ = burn.is_inertially_fixed;
    duration_ = manoeuvre.duration;
    initial_mass_in_tonnes_ = manoeuvre.initial_mass_in_tonnes;
  }

  public Burn Burn() {
    return new Burn{
        thrust_in_kilonewtons = thrust_in_kilonewtons_,
        specific_impulse_in_seconds_g0 = specific_impulse_in_seconds_g0_,
        frame = reference_frame_selector_.FrameParameters(),
        initial_time = previous_coast_duration_.value + time_base,
        delta_v = new XYZ{x = Δv_tangent_.value,
                          y = Δv_normal_.value,
                          z = Δv_binormal_.value},
        is_inertially_fixed = is_inertially_fixed_};
  }

  public void ReferenceFrameChanged(NavigationFrameParameters parameters) {
    changed_reference_frame_ = true;
  }

  public void Close() {
    reference_frame_selector_.DisposeWindow();
  }

  private void ComputeEngineCharacteristics() {
    ModuleEngines[] active_engines =
        (from part in vessel_.parts
         select (from PartModule module in part.Modules
                 where module is ModuleEngines &&
                       (module as ModuleEngines).EngineIgnited
                 select module as ModuleEngines)).SelectMany(x => x).ToArray();
    Vector3d reference_direction = vessel_.ReferenceTransform.up;
    double[] thrusts =
        (from engine in active_engines
         select engine.maxThrust *
             (from transform in engine.thrustTransforms
              select Math.Max(0,
                              Vector3d.Dot(reference_direction,
                                           -transform.forward))).Average()).
            ToArray();
    thrust_in_kilonewtons_ = thrusts.Sum();

    // This would use zip if we had 4.0 or later.  We loop for now.
    double Σ_f_over_i_sp = 0;
    for (int i = 0; i < active_engines.Count(); ++i) {
      Σ_f_over_i_sp +=
          thrusts[i] / active_engines[i].atmosphereCurve.Evaluate(0);
    }
    specific_impulse_in_seconds_g0_ = thrust_in_kilonewtons_ / Σ_f_over_i_sp;

    // If there are no engines, fall back onto RCS.
    if (thrust_in_kilonewtons_ == 0) {
      engine_warning_ += "No active engines, falling back to RCS. ";
      ComputeRCSCharacteristics();
    }
  }

  private void ComputeRCSCharacteristics() {
    ModuleRCS[] active_rcs =
        (from part in vessel_.parts
         select (from PartModule module in part.Modules
                 where module is ModuleRCS &&
                       (module as ModuleRCS).rcsEnabled
                 select module as ModuleRCS)).SelectMany(x => x).ToArray();
    Vector3d reference_direction = vessel_.ReferenceTransform.up;
    // NOTE(egg): NathanKell informs me that in >= 1.0.5, RCS has a useZaxis
    // property, that controls whether they thrust -up or -forward.  The madness
    // keeps piling up.
    double[] thrusts =
        (from engine in active_rcs
         select engine.thrusterPower *
             (from transform in engine.thrusterTransforms
              select Math.Max(0,
                              Vector3d.Dot(reference_direction,
                                           -transform.up))).Average()).
            ToArray();
    thrust_in_kilonewtons_ = thrusts.Sum();

    // This would use zip if we had 4.0 or later.  We loop for now.
    double Σ_f_over_i_sp = 0;
    for (int i = 0; i < active_rcs.Count(); ++i) {
      Σ_f_over_i_sp +=
          thrusts[i] / active_rcs[i].atmosphereCurve.Evaluate(0);
    }
    specific_impulse_in_seconds_g0_ = thrust_in_kilonewtons_ / Σ_f_over_i_sp;

    // If RCS provides no thrust, model a virtually instant burn.
    if (thrust_in_kilonewtons_ == 0) {
      engine_warning_ += "No active RCS, modeling as instant burn. ";
      UseTheForceLuke();
    }
  }

  private void UseTheForceLuke() {
    // The burn can last at most (9.80665 / scale) s.
    const double scale = 1;
    // This, together with |scale = 1|, ensures that, when |initial_time| is
    // less than 2 ** 32 s, |Δv(initial_time + duration)| does not overflow if
    // Δv is less than 100 km/s, and that |initial_time + duration| does not
    // fully cancel if Δv is more than 1 mm/s.
    // TODO(egg): Before the C* release, add a persisted flag to indicate to the
    // user that we are not using the craft's engines (we can also use that
    // flag to remember whether the burn was created for active engines or
    // active RCS).
    const double range = 1000;
    thrust_in_kilonewtons_ = initial_mass_in_tonnes_ * range * scale;
    specific_impulse_in_seconds_g0_ = range;
  }

  internal string FormatPreviousCoastDuration(double value) {
    return FlightPlanner.FormatPositiveTimeSpan(TimeSpan.FromSeconds(value));
  }

  internal bool TryParsePreviousCoastDuration(string str, out double value) {
    value = 0;
    TimeSpan ts;
    if (!FlightPlanner.TryParseTimeSpan(str, out ts)) {
      return false;
    }
    value = ts.TotalSeconds;
    return true;
  }

  private double time_base => previous_burn_?.final_time ??
                              plugin.FlightPlanGetInitialTime(
                                  vessel_.id.ToString());

  private double final_time =>
      time_base + previous_coast_duration_.value + duration_;

  private IntPtr plugin => adapter_.Plugin();

  private bool is_inertially_fixed_;
  private DifferentialSlider Δv_tangent_;
  private DifferentialSlider Δv_normal_;
  private DifferentialSlider Δv_binormal_;
  private DifferentialSlider previous_coast_duration_;
  private ReferenceFrameSelector reference_frame_selector_;
  private double thrust_in_kilonewtons_;
  private double specific_impulse_in_seconds_g0_;
  private double duration_;
  private double initial_mass_in_tonnes_;

  private bool first_time_rendering = true;

  private const double Log10ΔvLowerRate = -3.0;
  private const double Log10ΔvUpperRate = 3.5;
  private const double Log10TimeLowerRate = 0.0;
  private const double Log10TimeUpperRate = 7.0;

  // Not owned.
  private readonly Vessel vessel_;
  private readonly int index_;
  private readonly BurnEditor previous_burn_;
  private readonly PrincipiaPluginAdapter adapter_;

  private bool changed_reference_frame_ = false;
  private string engine_warning_ = "";
}

}  // namespace ksp_plugin_adapter
}  // namespace principia
