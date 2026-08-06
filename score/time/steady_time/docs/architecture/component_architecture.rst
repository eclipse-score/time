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

.. _steady_time_component_architecture:

SteadyTime Architecture Documentation
=====================================

.. document:: SteadyTime Architecture
   :id: doc__steady_time_architecture
   :status: draft
   :version: 1
   :safety: QM
   :security: NO
   :realizes: wp__component_arch
   :tags: steady_time

Overview
--------

<Brief summary of the architecture.>

Static Architecture
-------------------

.. comp:: SteadyTime
   :id: comp__steady_time
   :security: NO
   :safety: QM
   :status: invalid
   :version: 1
   :belongs_to: feat__time

.. comp_arc_sta:: SteadyTime (Static View)
   :id: comp_arc_sta__steady_time__sv
   :security: NO
   :safety: QM
   :status: invalid
   :version: 1
   :belongs_to: comp__steady_time
   :fulfils:

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

.. comp_arc_dyn:: SteadyTime Dynamic View
   :id: comp_arc_dyn__steady_time__dv
   :security: NO
   :safety: QM
   :status: invalid
   :version: 1
   :belongs_to: comp__steady_time
   :fulfils:

   Put here a sequence diagram
