// Ecossistema SisTer — MIRANTE v0.1.0
// Client-side Application Logic

document.addEventListener('DOMContentLoaded', () => {
  let activeSessionId = null;
  let currentSessionData = null;
  let rivalsList = [];
  let risksList = [];
  let pendingAssistantText = "";

  const API_BASE = ""; // Relative to server root

  // Elementos UI
  const sessionBreadcrumb = document.getElementById('sessionBreadcrumb');
  const currentSessionIdEl = document.getElementById('currentSessionId');
  const cycleBadge = document.getElementById('cycleBadge');
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');
  const activeModelName = document.getElementById('activeModelName');

  // Navegação Lateral
  const navItems = document.querySelectorAll('.sidebar-menu .nav-item');
  const sections = {
    '#workbench': document.getElementById('sectionWorkbench'),
    '#confrontation': document.getElementById('sectionConfrontation'),
    '#reposition': document.getElementById('sectionReposition'),
    '#provenance': document.getElementById('sectionProvenance'),
    '#dossier': document.getElementById('sectionDossier')
  };

  navItems.forEach(item => {
    item.addEventListener('click', (e) => {
      e.preventDefault();
      const targetHash = item.getAttribute('href');
      switchSection(targetHash);
    });
  });

  function switchSection(targetHash) {
    navItems.forEach(i => i.classList.remove('active'));
    Object.values(sections).forEach(s => {
      if (s) {
        s.classList.remove('active-section');
        s.style.display = 'none';
      }
    });

    const activeNav = document.querySelector(`.sidebar-menu .nav-item[href="${targetHash}"]`);
    if (activeNav) activeNav.classList.add('active');

    const targetSec = sections[targetHash];
    if (targetSec) {
      targetSec.classList.add('active-section');
      targetSec.style.display = 'block';
    }

    if (targetHash === '#dossier') renderDossier();
    if (targetHash === '#provenance') renderProvenanceLedger();
  }

  // Trilha de Abas M1, M2, M3
  const trackTabs = document.querySelectorAll('.track-tab');
  const tabContents = document.querySelectorAll('.tab-content');

  trackTabs.forEach(tab => {
    tab.addEventListener('click', () => {
      const tabId = tab.dataset.tab;
      trackTabs.forEach(t => t.classList.remove('active'));
      tabContents.forEach(c => c.classList.remove('active'));

      tab.classList.add('active');
      const targetContent = document.getElementById(tabId);
      if (targetContent) targetContent.classList.add('active');
    });
  });

  // Drawer de Assistência
  const assistantDrawer = document.getElementById('assistantDrawer');
  const btnCloseDrawer = document.getElementById('btnCloseDrawer');
  const drawerContextInfo = document.getElementById('drawerContextInfo');
  const drawerLoading = document.getElementById('drawerLoading');
  const suggestionBox = document.getElementById('suggestionBox');
  const sugText = document.getElementById('sugText');
  const sugModelName = document.getElementById('sugModelName');

  btnCloseDrawer.addEventListener('click', () => {
    assistantDrawer.classList.remove('open');
  });

  function openAssistant(contextLabel) {
    drawerContextInfo.textContent = `Contexto: ${contextLabel}`;
    suggestionBox.style.display = 'none';
    drawerLoading.style.display = 'none';
    assistantDrawer.classList.add('open');
  }

  // Verificar Saúde e Iniciar
  async function init() {
    try {
      const hRes = await fetch(`${API_BASE}/api/health`);
      if (hRes.ok) {
        const hData = await hRes.json();
        statusDot.classList.add('online');
        statusText.textContent = hData.ollama_online ? "C++26 & Ollama Ativos" : "C++26 Ativo (Modo Humano)";
      }

      const mRes = await fetch(`${API_BASE}/api/models`);
      if (mRes.ok) {
        const models = await mRes.json();
        if (models.length > 0) {
          activeModelName.textContent = models[0].name;
        }
      }

      await loadOrInitSession();
    } catch (err) {
      console.error("Erro na inicialização:", err);
      statusDot.classList.remove('online');
      statusText.textContent = "Conexão Offline";
    }
  }

  async function loadOrInitSession() {
    const res = await fetch(`${API_BASE}/api/sessions`);
    const sessions = await res.json();
    if (sessions && sessions.length > 0) {
      activeSessionId = sessions[0].session_id;
    } else {
      // Cria sessão canônica piloto do Resiliência
      const createRes = await fetch(`${API_BASE}/api/sessions`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          session_id: "SES-2026-10-RES01",
          project_id: "PRJ-RESILIENCIA",
          title: "Relações ainda não evidentes no Projeto Resiliência",
          entrance_door: "PATH_B_PHENOMENON_TO_KNOWLEDGE",
          phenomenon_text: "Sistemas agropecuários familiares no bioma Pampa sujeitos à mesma estiagem severa desenvolvem trajetórias profundamente discrepantes de recuperação forrageira e financeira.",
          author: "José Pedro Trindade"
        })
      });
      const cData = await createRes.json();
      activeSessionId = cData.session_id;
    }

    await fetchSessionDetails(activeSessionId);
  }

  async function fetchSessionDetails(sid) {
    const res = await fetch(`${API_BASE}/api/sessions/${sid}`);
    if (res.ok) {
      currentSessionData = await res.json();
      currentSessionIdEl.textContent = currentSessionData.session_id;
      sessionBreadcrumb.textContent = `${currentSessionData.project_id} / ${currentSessionData.title}`;
      cycleBadge.textContent = `Ciclo ${currentSessionData.current_cycle}`;

      if (currentSessionData.phenomenon) {
        document.getElementById('m1InitialFw').value = currentSessionData.phenomenon.text || "";
      }

      if (currentSessionData.shift) {
        document.getElementById('m1ShiftedFw').value = currentSessionData.shift.shifted || "";
        document.getElementById('m1Rationale').value = currentSessionData.shift.rationale || "";
      }
    }
  }

  // Gravar M1 (Deslocamento)
  document.getElementById('btnSaveM1').addEventListener('click', async () => {
    const initialFw = document.getElementById('m1InitialFw').value.trim();
    const shiftedFw = document.getElementById('m1ShiftedFw').value.trim();
    const rationale = document.getElementById('m1Rationale').value.trim();
    const dimension = document.getElementById('m1Dimension').value;

    const res = await fetch(`${API_BASE}/api/sessions/${activeSessionId}/shift`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        initial_framework: initialFw,
        shifted_framework: shiftedFw,
        rationale: rationale,
        dimension: dimension,
        author: "José Pedro Trindade",
        is_assisted: false
      })
    });

    if (res.ok) {
      alert("✅ Deslocamento M1 consolidado com sucesso no ciclo!");
      // Avança para aba M2
      document.querySelector('.track-tab[data-tab="tabM2"]').click();
    } else {
      const err = await res.json();
      alert(`⚠️ Violação de Invariante (${err.invariant || 'Erro'}): ${err.error}\n\nDica: ${err.hint || ''}`);
    }
  });

  // Gravar M2 (Relação Candidata)
  document.getElementById('btnSaveM2').addEventListener('click', async () => {
    const termA = document.getElementById('m2TermA').value.trim();
    const termB = document.getElementById('m2TermB').value.trim();
    const desc = document.getElementById('m2Description').value.trim();
    const relType = document.getElementById('m2RelType').value;

    const res = await fetch(`${API_BASE}/api/sessions/${activeSessionId}/relation`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        term_a: termA,
        term_b: termB,
        description: desc,
        rel_type: relType,
        author: "José Pedro Trindade",
        is_assisted: false
      })
    });

    if (res.ok) {
      const listEl = document.getElementById('relationsList');
      const item = document.createElement('div');
      item.className = 'stack-item';
      item.innerHTML = `<strong>${termA} &harr; ${termB}</strong> <span class="provenance-tag candidate">[CANDIDATA]</span><br><small>${desc}</small>`;
      listEl.appendChild(item);

      document.getElementById('m2TermA').value = "";
      document.getElementById('m2TermB').value = "";
      document.getElementById('m2Description').value = "";

      alert("✅ Relação candidata registrada no inventário.");
    } else {
      const err = await res.json();
      alert(`⚠️ Violação de Invariante (${err.invariant || 'Erro'}): ${err.error}\n\nDica: ${err.hint || ''}`);
    }
  });

  // Gravar M3 (Problematização)
  document.getElementById('btnSaveM3').addEventListener('click', async () => {
    const qtext = document.getElementById('m3Question').value.trim();
    const evPath = document.getElementById('m3Evidence').value.trim();

    const res = await fetch(`${API_BASE}/api/sessions/${activeSessionId}/question`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        question_text: qtext,
        evidence_path: evPath,
        author: "José Pedro Trindade",
        is_assisted: false
      })
    });

    if (res.ok) {
      alert("✅ Questão formulada. Avançando para a Arena de Confronto (C4)!");
      switchSection('#confrontation');
    } else {
      const err = await res.json();
      alert(`⚠️ Violação de Invariante (${err.invariant || 'Erro'}): ${err.error}\n\nDica: ${err.hint || ''}`);
    }
  });

  // Confronto (C4): Adicionar Rival
  document.getElementById('btnAddRival').addEventListener('click', () => {
    const text = document.getElementById('c4RivalText').value.trim();
    if (!text) return;

    rivalsList.push({ text: text, author: "Revisor Crítico" });
    const stack = document.getElementById('rivalsStack');
    const el = document.createElement('div');
    el.className = 'stack-item';
    el.innerHTML = `<strong>Rival ${rivalsList.length}:</strong> ${text}`;
    stack.appendChild(el);

    document.getElementById('c4RivalText').value = "";
    document.getElementById('rivalCount').textContent = `${rivalsList.length} / 1`;
    checkConfrontationReadiness();
  });

  // Confronto (C4): Adicionar Risco
  document.getElementById('btnAddRisk').addEventListener('click', () => {
    const text = document.getElementById('c4RiskText').value.trim();
    if (!text) return;

    risksList.push({ text: text, author: "Revisor Crítico" });
    const stack = document.getElementById('risksStack');
    const el = document.createElement('div');
    el.className = 'stack-item';
    el.innerHTML = `<strong>Risco ${risksList.length}:</strong> ${text}`;
    stack.appendChild(el);

    document.getElementById('c4RiskText').value = "";
    document.getElementById('riskCount').textContent = `${risksList.length} / 1`;
    checkConfrontationReadiness();
  });

  function checkConfrontationReadiness() {
    const btn = document.getElementById('btnConcludeConfrontation');
    if (rivalsList.length >= 1 && risksList.length >= 1) {
      btn.disabled = false;
    } else {
      btn.disabled = true;
    }
  }

  // Concluir Confronto
  document.getElementById('btnConcludeConfrontation').addEventListener('click', async () => {
    const res = await fetch(`${API_BASE}/api/sessions/${activeSessionId}/confront`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        rivals: rivalsList,
        risks: risksList
      })
    });

    if (res.ok) {
      alert("✅ Confronto Crítico concluído com êxito! Habilitando Reposicionamento (C5).");
      switchSection('#reposition');
    } else {
      const err = await res.json();
      alert(`⚠️ Erro no Confronto: ${err.error}`);
    }
  });

  // Reposicionamento (C5): Gravar
  document.getElementById('btnConsolidateReposition').addEventListener('click', async () => {
    const decision = document.querySelector('input[name="repositionDecision"]:checked').value;
    const just = document.getElementById('c5Justification').value.trim();
    const nextTest = document.getElementById('c5NextTest').value.trim();
    const reqKnowledge = document.getElementById('c5ReqKnowledge').value.trim();

    const res = await fetch(`${API_BASE}/api/sessions/${activeSessionId}/reposition`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        decision: decision,
        justification: just,
        next_empirical_test: nextTest,
        required_knowledge: reqKnowledge,
        resumption_criteria: (decision === 'SUSPEND') ? "Aguardando novos levantamentos empíricos" : "",
        author: "José Pedro Trindade"
      })
    });

    if (res.ok) {
      alert("🎉 Ciclo MPRR consolidado e selado com hash criptográfico no Ledger de Auditoria!");
      switchSection('#dossier');
    } else {
      const err = await res.json();
      alert(`⚠️ Violação de Invariante (${err.invariant || 'Erro'}): ${err.error}\n\nDica: ${err.hint || ''}`);
    }
  });

  // Assistência Local (Ollama): M1
  document.getElementById('btnAssistShift').addEventListener('click', async () => {
    openAssistant("M1 Deslocamento de Perspectiva");
    drawerLoading.style.display = 'flex';

    try {
      const res = await fetch(`${API_BASE}/api/assist/shift`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          phenomenon: document.getElementById('m1InitialFw').value,
          current_framework: "Foco exclusivo na precipitação anual e taxa de lotação animal"
        })
      });
      const data = await res.json();
      drawerLoading.style.display = 'none';

      if (data.suggestion) {
        pendingAssistantText = data.suggestion;
        sugText.textContent = data.suggestion;
        sugModelName.textContent = data.model;
        suggestionBox.style.display = 'block';
      }
    } catch (e) {
      drawerLoading.style.display = 'none';
      alert("Assistência local offline ou indisponível.");
    }
  });

  // Assistência Local: Advogado do Diabo (C4)
  document.getElementById('btnDevilsAdvocate').addEventListener('click', async () => {
    const box = document.getElementById('devilsResponseBox');
    box.innerHTML = '<em>Consultando motor de ceticismo do modelo local...</em>';

    try {
      const res = await fetch(`${API_BASE}/api/assist/confront`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          hypothesis: "A resiliência das pastagens depende da memória biológica do solo acumulada na década anterior."
        })
      });
      const data = await res.json();
      if (data.suggestion) {
        box.innerHTML = `<div style="white-space: pre-wrap; font-size: 13px; line-height: 1.6;">${data.suggestion}</div>`;
      }
    } catch (e) {
      box.innerHTML = '<em>Assistência crítica indisponível. Preencha manualmente os campos acima.</em>';
    }
  });

  // Ações sobre a Sugestão do Assistente (Os 4 Botões do MPRR)
  document.getElementById('btnAcceptSug').addEventListener('click', () => {
    document.getElementById('m1ShiftedFw').value = pendingAssistantText;
    document.getElementById('m1Rationale').value = "Incorporado a partir da exploração assistida pelo modelo local.";
    assistantDrawer.classList.remove('open');
    alert("Sugestão aceita como rascunho com tag de proveniência [ASSISTIDO].");
  });

  document.getElementById('btnEditSug').addEventListener('click', () => {
    document.getElementById('m1ShiftedFw').value = pendingAssistantText;
    assistantDrawer.classList.remove('open');
    document.getElementById('m1ShiftedFw').focus();
  });

  document.getElementById('btnRejectSug').addEventListener('click', () => {
    suggestionBox.style.display = 'none';
    assistantDrawer.classList.remove('open');
  });

  document.getElementById('btnCounterSug').addEventListener('click', () => {
    alert("Solicitando contraponto crítico socrático à própria proposição...");
  });

  // Renderizar Dossiê
  async function renderDossier() {
    if (!activeSessionId) return;
    const res = await fetch(`${API_BASE}/api/sessions/${activeSessionId}`);
    if (!res.ok) return;
    const s = await res.json();

    document.getElementById('metaId').textContent = s.session_id;
    document.getElementById('sheetPhenomenon').textContent = s.phenomenon ? s.phenomenon.text : "--";
    document.getElementById('sheetInitialFw').textContent = s.phenomenon ? s.phenomenon.text : "--";
    document.getElementById('sheetShiftedFw').textContent = s.shift ? `${s.shift.shifted} (Dimensão: ${s.shift.dimension})` : "--";
    document.getElementById('sheetRelations').textContent = `${s.relations_count || 0} relação(ões) candidata(s) catalogada(s)`;
    document.getElementById('sheetQuestions').textContent = `${s.questions_count || 0} questão(ões) investigável(is) gerada(s)`;
    document.getElementById('sheetConfrontation').textContent = s.confrontation_done ? "Confronto concluído: Explicações rivais e riscos de invalidação registrados." : "Pendente de confronto obrigatório";
    document.getElementById('sheetReposition').textContent = `Ciclo ${s.current_cycle} em consolidação`;
    document.getElementById('sheetProvenance').textContent = "Pesquisador: José Pedro Trindade | Núcleo: C++26 | IA: Ollama Local (Auditado)";
  }

  // Exportar .sister-research.json
  document.getElementById('btnExportJson').addEventListener('click', async () => {
    const res = await fetch(`${API_BASE}/api/sessions/${activeSessionId}`);
    const s = await res.json();
    const manifest = {
      document_id: `MPRR-${s.session_id}`,
      version: `0.${s.current_cycle}.0`,
      date: "2026-10-23",
      document_kind: "mprr_research_session",
      status: "procedurally_verified_unvalidated_empirically",
      publication_intent: "internal_scientific_record",
      method_reference: "MPRR-FOUNDATION-001@0.1.0",
      research_project_reference: s.project_id,
      session_data: s
    };

    const blob = new Blob([JSON.stringify(manifest, null, 2)], { type: 'application/json' });
    const url = URL.createObjectURL(blob);
    const a = document.createElement('a');
    a.href = url;
    a.download = `${s.session_id}.sister-research.json`;
    a.click();
  });

  init();
});
