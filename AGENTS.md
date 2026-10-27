# AGENTS.md — Wiper Control System

## 1. Project Overview
- Purpose: rain-adaptive wiper control combining driver input and rain-sensor feedback to control wiper motor speed (PRD Sections 1-2)
- Driver modes: OFF, ON/continuous, intermittent (configurable interval), automatic (PR-001 to PR-004)
- System boundaries: mechanical linkage, motor-driver power electronics, vehicle-network protocol, and vehicle homologation are out of scope until specified (PRD Section 3)
- Supported motor variants: Small, Medium, Large (S/M/L)
- Target MCU: Renesas RH850 family (demo default); exact part number pending MCU selection (PR-024)
- Control-loop target: 500 Hz (2 ms period); this is a task execution rate, not a wiper wiping frequency
- Project status: production ECU development; the standards in Section 7 are target standards
- Items tagged "demo default" are placeholder choices for this demo; confirm or replace them before production use

## 2. Requirements & Traceability
- Source of truth: Product Requirements Document (PRD)
- Requirement ID traceability (PR-xxx)
- Link requirements to design, implementation, and tests
- Do not invent unspecified hardware parameters
- Do not assume an ASIL, cybersecurity assurance level (CAL), or vehicle-network interface; they come from the hazard analysis / TARA and system architecture
- Safety and security requirements get their own stable IDs (e.g., SR-xxx, SEC-xxx) and trace to PR-xxx and to the hazard/threat they mitigate

## 3. Architecture & Design
- Driver input handling (ON/OFF, intermittent timing): deterministic priority between driver commands and automatic control; input filtering/debounce (PR-005 to PR-007)
- Rain-sensor acquisition and intensity estimation: calibrated mapping with thresholds, hysteresis, and filtering; stale/implausible sensor detection and documented fallback (PR-010 to PR-015)
- Adaptive wiper control logic: sensor intensity is never a direct motor-speed command unless calibration defines it (PR-015)
- Motor driver interface and feedback (PR-022)
- Thermal management and fault handling (PR-050 to PR-056, PR-060 to PR-064)
- MCU memory and resource allocation: RAM/flash budget, persistent-settings integrity, and isolation of optional workloads (PR-031 to PR-034)
- Decisions (demo defaults, pending system architecture): vehicle-network interface is LIN; park position is the end-of-sweep position confirmed by position feedback; sensor-to-command latency target is 20 ms; MCU part number is still unselected

## 4. Coding Standards
- Programming language: C99 (demo default) with a GCC-based cross compiler for the target MCU; pin the exact compiler version in `docs/design/`
- Secure coding standard: MISRA C:2012 (incl. Amendments); the exact edition and amendments are recorded in the compliance matrix
- Naming conventions and code formatting enforced by an automated formatter config checked into the repo
- Modular design and separation of concerns (HAL / drivers / control logic / diagnostics / application)
- Error handling and defensive programming: check all return values, validate all inputs at module boundaries, no silent failures
- Static analysis and compiler warning policy: warnings are errors; new analyzer findings are fixed or recorded as reviewed deviations before merge
- Avoid dynamic memory allocation where required by project constraints; no recursion, no unbounded loops in the control path
- Deviations from MISRA rules require a documented, reviewed deviation record (rule, rationale, scope, approver); no blanket suppressions
- Use fixed-width integer types, explicit casts, and bounded buffers; no undefined/unspecified/implementation-defined behavior reliance
- Safety-related and non-safety code must be separated so freedom from interference can be argued (see Section 8)

## 5. Real-Time Constraints
- Maintain 500 Hz control-task execution (PR-020)
- Enforce 2 ms deadline under worst-case workload with a 25% timing margin (demo default), i.e. WCET of at most 1.5 ms (PR-021)
- Prevent blocking operations in time-critical tasks
- Isolate background application workloads so they cannot cause deadline misses (PR-025, PR-034)
- Use watchdog and overrun detection with defined behavior for overrun, lockup, or reset (PR-026)
- Measure or estimate task execution time and expose timing-fault diagnostics (PR-027)

## 6. Motor Safety & Limits
- Enforce configured torque, current, speed, and thermal limits
- Validate S/M/L motor configuration
- Handle stall, overload, and sensor failures
- Never assume motor ratings; use approved specifications; S/M/L parameter sets come from approved supplier and vehicle data (PR-040, PR-041, PRD Section 8)
- Do not use the S/M/L label alone to infer safe limits; invalid or unsupported configuration must not enable unrestricted motor operation (PR-046)
- Enforce the eight-hour continuous-operation constraint as a duty-cycle/thermal policy, not an arbitrary shutdown that creates an unsafe visibility condition (PR-050, PR-051)
- Define safe startup, shutdown, and recovery behavior
- Avoid unintended motor activation during startup, reset, brownout, or configuration failure (PR-063)

## 7. Applicable Standards

| Domain | Standard | Project use |
|---|---|---|
| Functional safety | ISO 26262 | Target standard; HARA determines the ASIL; safety goals, safety requirements, and safety case follow from it |
| Cybersecurity | ISO/SAE 21434 | Threat analysis and risk assessment (TARA), cybersecurity goals/requirements, secure design, vulnerability management |
| Software updates | ISO 24089 | Software update engineering: package integrity, authenticity, compatibility, rollback, and traceability |
| Process and quality | Automotive SPICE (PAM v4.0, demo default) | Process evidence: bidirectional traceability, reviews, verification strategy, change and configuration management |
| Secure coding | MISRA C / MISRA C++ | Coding rules, compliance matrix, deviation records |

- Standards are normative inputs; where this file and a standard disagree, the standard and the approved project plans win, and the conflict must be raised for review
- The PRD (v1.0) still lists standards allocation as out of scope; update the PRD to record these as target standards
- Maintain a compliance matrix in `docs/compliance/` mapping each applicable clause/work product to its repo artifact
- Vehicle-level regulations (e.g., UNECE R155/R156) are out of scope until the vehicle program confirms applicability
- Do not claim compliance or certification; Copilot output is a development aid, not evidence on its own

## 8. Functional Safety (ISO 26262)
- ASIL is determined by hazard analysis (loss of wiping, unintended wiping, thermal shutdown affecting visibility, etc.); never assume or hard-code an ASIL
- Until the ASIL is allocated, treat motor-limit, timing, watchdog, and fault-handling code as safety-related and request review
- Derive software safety requirements from safety goals; trace each to design, code, and verification (SR-xxx -> design -> code -> test)
- Implement safety mechanisms defined by the safety concept: input plausibility checks, watchdog, program-flow/timing monitoring, memory and configuration integrity checks, safe-state transitions
- Define the safe state(s) and fault-tolerant time interval for each fault (PR-060 to PR-062); consider driver visibility before shutting down (PR-051, PR-055)
- Maintain freedom from interference between safety-related and other software (memory partitioning, scheduling, no shared mutable state without protection)
- Keep safety analyses (FMEA/FTA/DFA, as applicable) current when design changes
- Tool qualification: document confidence in compilers, analyzers, and test tools used for safety evidence
- Safety-related changes require impact analysis and review before merge, with the independence required by the allocated ASIL (see Section 13)

## 9. Cybersecurity (ISO/SAE 21434)
- Maintain a TARA; derive cybersecurity requirements (SEC-xxx) and trace them to design and tests
- Define trust boundaries: vehicle-network interface, diagnostic interface, debug ports, programming interface, calibration data
- Validate and bound-check all external inputs (network messages, diagnostics, calibration, configuration, sensor data)
- Authenticate and authorize diagnostic, calibration, and update access; disable or lock debug/test interfaces in production
- Use approved, maintained cryptographic libraries and hardware security features; never implement custom cryptography
- Protect integrity of persistent settings and calibration data (PR-033); fail to safe defaults on tamper or corruption
- Secure boot / authenticated firmware is required if the TARA and selected MCU support it; record the decision if not used
- Track third-party and SDK components in an SBOM; monitor and triage vulnerabilities; record security-relevant events without leaking sensitive data
- Security findings affecting safety behavior are escalated to the functional-safety review

## 10. Software Updates (ISO 24089)
- Update mechanism (demo default): wired update through the diagnostic interface and bootloader; no OTA
- The PRD has no software-update requirements yet; update requirement IDs are to be added to the PRD, and code in this area traces to them once they exist
- Every release has a unique version, build identifier, and compatibility declaration (hardware, motor variant, calibration)
- Update packages are signed and verified before installation; reject unsigned, malformed, downgrade (unless approved), or incompatible packages
- Updates must be atomic and recoverable: power loss or failure during update must not leave the motor in an unsafe or unbootable state
- No unintended motor activation while an update is in progress; define the vehicle conditions in which updating is allowed
- Keep a rollback / fallback image strategy and verify post-update integrity and configuration validity
- Record update history for traceability; update documentation and SBOM per release

## 11. Sensitive Data Handling
- Never commit secrets: signing/private keys, certificates, tokens, passwords, production credentials, debug/unlock keys, or seeds
- Do not read or edit `.env*` files or other secret stores except `.env.example`; use placeholders in examples and docs
- Signing keys are never in the repo, logs, or artifacts; key custody (demo default) is GitHub Actions encrypted secrets, with the procedure recorded in `docs/security/`
- Treat the following as confidential: supplier datasheets under NDA, motor/driver calibration data, vehicle-OEM specifications, proprietary algorithms, diagnostic security-access algorithms, vulnerability reports and TARA results
- Personal data: this system is not expected to collect any; if logging or diagnostics could include VIN, location, or driver identifiers, apply data minimization, access control, and retention limits, and record the decision
- Redact sensitive values in logs, crash dumps, test reports, and Copilot prompts; do not paste secrets or confidential data into external tools
- Store production calibration and keys outside the source tree; protect with least-privilege access
- Report suspected secret exposure immediately and rotate the affected material

## 12. Testing & Verification
- Unit tests for control logic and state transitions
- Boundary-value and fault-injection tests
- Sensor failure and invalid-input tests
- Timing and worst-case execution-time tests
- Motor variant and thermal-limit tests
- Requirement-to-test traceability
- Structural coverage targets (e.g., statement/branch/MC/DC) are set per ASIL once allocated; record the target and measured result
- Security tests: fuzzing of external interfaces, authentication/authorization negative tests, update-package tampering tests, debug-lock verification
- Regression suite runs on every change; flaky tests are fixed, not ignored
- Verification records are kept with version, configuration, and tool identifiers as Automotive SPICE evidence

## 13. Production Engineering
- Static analysis: Cppcheck with its MISRA add-on plus clang-tidy on every change (demo defaults); record tool versions in `tools/`
- Traceability: bidirectional PRD -> safety/security requirement -> design -> code -> test -> result, kept current and automatically checked where possible
- Fault injection: sensor, motor, thermal, memory/configuration, watchdog, power, and communication faults, run in simulation and on HIL
- Real-time validation: measured and analyzed WCET for the 500 Hz task, jitter, stack and memory usage, and overrun detection with recorded margin
- Secure release management: tagged, reproducible builds; signed artifacts; SBOM; release notes with known issues; approval gate before release; archived build and test evidence
- Human review: review for every change; changes touching motor limits, timing, fault handling, update, or crypto get safety and security review with the independence required by the allocated ASIL and security plan; if the team is too small to provide it, record the reduced independence as an approved project decision and do not claim the independence; AI-generated code is reviewed like any other code
- Configuration and change management: baselines, controlled calibration changes, and impact analysis recorded per change

## 14. Build & Development Workflow
- Toolchain: GCC-based cross compiler with the Renesas RH850 SDK (demo defaults); pin exact versions and checksums
- Build, test, lint, and static-analysis commands: define in `tools/` and `README.md` once the toolchain is selected; do not guess commands
- Hardware-in-the-loop (HIL) requirements, if applicable
- CI/CD validation requirements: build, unit tests, static analysis, traceability check, SBOM generation, and secret scanning must pass before merge
- Source hosting and CI: GitHub with GitHub Actions; branch protection with pull requests only and required reviews
- No unverified build or test success claims

## 15. Documentation
- Document interfaces, assumptions, and design decisions
- Maintain requirement and test traceability
- Record configuration parameters and their sources
- Update documentation when behavior changes
- Keep safety case, TARA, compliance matrix, deviation records, and release notes in the locations defined in Section 16

## 16. Recommended Repository Structure

```text
wiper-control-system/
├── AGENTS.md
├── README.md
├── docs/
│   ├── wiper_control_system_product_requirements.md   # PRD (source of truth)
│   ├── architecture/        # system/software architecture, interfaces, memory and timing budgets
│   ├── design/              # detailed design, state machines, control and thermal models
│   ├── safety/              # HARA, safety goals, safety requirements, FMEA/FTA, safety case
│   ├── security/            # TARA, cybersecurity requirements, threat models, SBOM policy
│   ├── compliance/          # standards compliance matrix, MISRA deviation records, ASPICE evidence index
│   ├── traceability/        # requirement -> design -> code -> test matrices
│   ├── verification/        # test plans, reports, coverage, WCET and fault-injection results
│   └── release/             # release notes, release checklists, update/rollback procedures
├── src/
│   ├── app/                 # application layer: mode manager, wiper sequencing
│   ├── control/             # rain-intensity mapping, adaptive control, motor control (500 Hz task)
│   ├── safety/              # watchdog, plausibility checks, limit enforcement, safe-state logic
│   ├── diagnostics/         # fault detection, logging, diagnostic reporting
│   ├── security/            # authentication, integrity checks, update verification
│   ├── update/              # update handler, bootloader interface, rollback
│   ├── config/              # S/M/L motor parameter sets, calibration, persistent-settings handling
│   ├── drivers/             # sensor, ADC, PWM, motor-driver, input drivers
│   ├── hal/                 # MCU abstraction (Renesas/TI specific code isolated here)
│   └── platform/            # startup, scheduler/RTOS config, linker scripts, memory map
├── include/                 # public headers
├── tests/
│   ├── unit/                # host-based unit tests
│   ├── integration/         # integration and SIL tests
│   ├── hil/                 # hardware-in-the-loop tests
│   ├── fault_injection/     # fault-injection scenarios
│   ├── timing/              # WCET, jitter, and overrun tests
│   ├── security/            # fuzzing, tamper, and access-control tests
│   └── mocks/               # HAL and driver mocks
├── tools/                   # build scripts, static-analysis configs, traceability and SBOM tooling
├── third_party/             # vendor SDK and external components with version and license records
├── .github/
│   ├── workflows/           # CI: build, test, static analysis, secret scan, SBOM
│   └── copilot-instructions.md   # optional; keep consistent with this file
├── .env.example             # placeholders only; real .env files are git-ignored
└── .gitignore
```

- Structure is a recommendation; adjust once language, toolchain, and MCU are selected, and record changes in `docs/architecture/`
- Keep MCU-specific code behind `src/hal/` so the control logic stays host-testable
- Build output goes in a git-ignored `build/` directory
- Never place keys, certificates, or production calibration data in this tree

## 17. GitHub Copilot Instructions
- Read the PRD and relevant project instructions before making changes
- Clarify ambiguous safety-critical requirements
- Make minimal, focused, reviewable changes
- Do not modify unrelated modules
- Explain assumptions and identify unresolved decisions
- Never bypass safety checks to make tests pass
- Run relevant tests and report actual results
- Request review for changes affecting motor limits, timing, or safety behavior
- Follow MISRA rules in all generated code; flag any needed deviation instead of silently introducing one
- Do not assume an ASIL, cybersecurity level, MCU part, or vehicle interface; flag as an open decision
- Do not generate custom cryptography or hard-code keys, credentials, or secrets
- Do not access `.env*` files (other than `.env.example`) or other sensitive files
- Keep requirement IDs (PR/SR/SEC) in code comments or commit messages where traceability is needed
- Request review, with the independence required by the safety and security plans, for changes touching safety, security, or update logic