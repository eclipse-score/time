..
   # *******************************************************************************
   # Copyright (c) 2026 Contributors to the Eclipse Foundation
   #
   # See the NOTICE file(s) distributed with this work for additional
   # information regarding copyright ownership.
   #
   # This program and the accompanying materials are made available under the
   # terms of the Apache License Version 2.0 which is available at
   # https://www.apache.org/licenses/LICENSE-2.0
   #
   # SPDX-License-Identifier: Apache-2.0
   # *******************************************************************************

.. _system_time_component_architecture:

SystemTime Architecture Documentation
=====================================

.. document:: SystemTime Architecture
   :id: doc__system_time_architecture
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__component_arch
   :tags: system_time

Overview
--------

<Brief summary of the architecture.>

Static Architecture
-------------------

.. comp:: SystemTime
   :id: comp__system_time
   :security: NO
   :safety: QM
   :status: invalid
   :version: 1
   :belongs_to: feat__time

.. comp_arc_sta:: SystemTime (Static View)
   :id: comp_arc_sta__system_time__sv
   :security: NO
   :safety: QM
   :status: invalid
   :version: 1
   :belongs_to: comp__system_time
   :fulfils:

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

.. comp_arc_dyn:: SystemTime Dynamic View
   :id: comp_arc_dyn__system_time__dv
   :security: NO
   :safety: QM
   :status: invalid
   :version: 1
   :belongs_to: comp__system_time
   :fulfils:

   Put here a sequence diagram
