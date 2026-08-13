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

.. _time_daemon_component_architecture:

Time Daemon Architecture Documentation
======================================

.. document:: Time Daemon Architecture
   :id: doc__time_daemon_architecture
   :status: draft
   :version: 1
   :safety: ASIL_B
   :security: NO
   :realizes: wp__component_arch
   :tags: time_daemon

.. note::
   Work in progress: structure, titles, and needs IDs only. Content and req/comp/feat traceability links to follow in later PRs in this stack.

.. attention::
    The above directive must be updated according to your needs.

    - Adjust ``status`` to be ``valid``
    - Adjust ``safety`` and ``tags`` according to your needs


Overview
--------

<Brief summary of the architecture.>

Requirements Linked to Component Architecture
---------------------------------------------

.. code-block:: none

   .. needtable:: Overview of Component Requirements
      :style: table
      :columns: title;id
      :filter: search("comp_arch_sta__archdes$", "fulfils_back")
      :colwidths: 70,30

Description
-----------

<General Description>

<Design Decisions - For the documentation of the decision the :need:`gd_temp__change_decision_record` can be used.>

<Design Constraints>

Rationale Behind Architecture Decomposition
*******************************************

Mandatory: A motivation for the decomposition or reason for not further splitting it into internal components.

.. note:: Common decisions across components / cross cutting concepts is at the higher level.

Static Architecture
-------------------

The components are designed to cover the expectations from the feature architecture
(i.e. if already exists a definition it should be taken over and enriched).

A component can optional also consist of lower level components to further structure the architecture. The component and its static views can also optionally use interfaces provided by other components.

.. comp:: Time Daemon
   :id: comp__time_daemon
   :security: YES
   :safety: ASIL_B
   :status: invalid
   :version: 1
   :belongs_to: feat__time

.. comp_arc_sta:: Time Daemon (Static View)
   :id: comp_arc_sta__time_daemon__sv
   :security: YES
   :safety: ASIL_B
   :status: invalid
   :version: 1
   :belongs_to: comp__time_daemon
   :fulfils: comp_req__time_daemon__some_title

   .. needarch::
      :scale: 50
      :align: center

      {{ draw_component(need(), needs) }}

Dynamic Architecture
--------------------

.. comp_arc_dyn:: Dynamic View
   :id: comp_arc_dyn__time_daemon__dv
   :security: YES
   :safety: ASIL_B
   :status: invalid
   :version: 1
   :belongs_to: comp__time_daemon
   :fulfils: comp_req__time_daemon__some_title

   Put here a sequence diagram

Interfaces
----------

.. code-block:: rst

   .. real_arc_int:: <Title>
      :id: real_arc_int__<component>__<Title>
      :security: <YES|NO>
      :safety: <QM|ASIL_B>
      :fulfils: <link to component requirement id>
      :language: cpp
