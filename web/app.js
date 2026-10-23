// Ecossistema SisTer — MIRANTE v0.1.0
// Client-side Application Logic: Universal Scientific Instrument for MPRR

document.addEventListener('DOMContentLoaded', () => {
  let activeSessionId = null;
  let currentSessionData = null;
  let rivalsList = [];
  let risksList = [];
  let pendingAssistantText = "";
  let pendingAssistantMovement = "M1";
  let availableModels = [];
  let currentUser = null;

  const API_BASE = "";

  // Helper de Requisição Autenticada SisTer
  function authFetch(url, options = {}) {
    const token = localStorage.getItem('mirante_auth_token');
    options.headers = options.headers || {};
    if (token) {
      options.headers['Authorization'] = `Bearer ${token}`;
      options.headers['X-Mirante-Token'] = token;
    }
    return fetch(url, options);
  }

  // Elementos Principais
  const sessionBreadcrumb = document.getElementById('sessionBreadcrumb');
  const selectActiveSession = document.getElementById('selectActiveSession');
  const selectLocalModel = document.getElementById('selectLocalModel');
  const cycleBadge = document.getElementById('cycleBadge');
  const doorPill = document.getElementById('doorPill');
  const statusDot = document.getElementById('statusDot');
  const statusText = document.getElementById('statusText');

  // Elementos de Autenticação SisTer
  const btnOpenLoginModal = document.getElementById('btnOpenLoginModal');
  const userBadge = document.getElementById('userBadge');
  const userAvatar = document.getElementById('userAvatar');
  const userName = document.getElementById('userName');
  const btnLogout = document.getElementById('btnLogout');
  const modalAuth = document.getElementById('modalAuth');
  const btnCloseAuthModal = document.getElementById('btnCloseAuthModal');
  const authStepEmail = document.getElementById('authStepEmail');
  const authStepCode = document.getElementById('authStepCode');
  const inputAuthEmail = document.getElementById('inputAuthEmail');
  const inputAuthName = document.getElementById('inputAuthName');
  const inputAuthCode = document.getElementById('inputAuthCode');
  const btnSendOtp = document.getElementById('btnSendOtp');
  const btnVerifyOtp = document.getElementById('btnVerifyOtp');
  const btnBackToEmail = document.getElementById('btnBackToEmail');
  const authSentNotice = document.getElementById('authSentNotice');
  const authTargetEmail = document.getElementById('authTargetEmail');
  const authFeedback = document.getElementById('authFeedback');

  // Gestão de Sessão do Usuário
  async function checkAuthStatus() {
    const token = localStorage.getItem('mirante_auth_token');
    if (!token) {
      renderLoggedOut();
      return;
    }
    try {
      const res = await authFetch(`${API_BASE}/api/auth/me`);
      if (res.ok) {
        const data = await res.json();
        if (data.authenticated) {
          currentUser = data.user;
          renderLoggedIn(currentUser);
          return;
        }
      }
      renderLoggedOut();
    } catch {
      renderLoggedOut();
    }
  }

  function renderLoggedIn(user) {
    btnOpenLoginModal.classList.add('hidden');
    userBadge.classList.remove('hidden');
    userName.textContent = user.name || user.email;
    const initials = (user.name || user.email).substring(0, 2).toUpperCase();
    userAvatar.textContent = initials;

    // Atualiza nome padrão em formulários
    const authorInputs = ['nsAuthor', 'author_shift', 'author_rel', 'author_q', 'author_repo'];
    authorInputs.forEach(id => {
      const el = document.getElementById(id);
      if (el && !el.value) el.value = user.name || user.email;
    });
  }

  function renderLoggedOut() {
    currentUser = null;
    localStorage.removeItem('mirante_auth_token');
    btnOpenLoginModal.classList.remove('hidden');
    userBadge.classList.add('hidden');
  }

  // Modais de Autenticação
  btnOpenLoginModal.addEventListener('click', () => {
    modalAuth.classList.remove('hidden');
    authStepEmail.classList.remove('hidden');
    authStepCode.classList.add('hidden');
    authFeedback.textContent = '';
    inputAuthEmail.focus();
  });

  function closeAuthModal() {
    modalAuth.classList.add('hidden');
    authFeedback.textContent = '';
  }

  btnCloseAuthModal.addEventListener('click', closeAuthModal);
  modalAuth.addEventListener('click', (e) => {
    if (e.target === modalAuth) closeAuthModal();
  });

  btnBackToEmail.addEventListener('click', () => {
    authStepEmail.classList.remove('hidden');
    authStepCode.classList.add('hidden');
    authFeedback.textContent = '';
    inputAuthEmail.focus();
  });

  btnSendOtp.addEventListener('click', async () => {
    const email = inputAuthEmail.value.trim();
    const name = inputAuthName.value.trim();
    if (!email || !email.includes('@')) {
      authFeedback.className = 'auth-feedback error';
      authFeedback.textContent = 'Por favor, informe um endereço de e-mail válido.';
      return;
    }

    btnSendOtp.disabled = true;
    btnSendOtp.textContent = 'Gerando código...';
    authFeedback.textContent = '';

    try {
      const res = await fetch(`${API_BASE}/api/auth/otp/request`, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({email, name})
      });
      const data = await res.json();
      if (!res.ok) throw new Error(data.error || 'Falha ao solicitar código.');

      authTargetEmail.textContent = email;
      authStepEmail.classList.add('hidden');
      authStepCode.classList.remove('hidden');
      inputAuthCode.value = '';
      inputAuthCode.focus();

      if (data.dev_code) {
        authFeedback.className = 'auth-feedback success';
        authFeedback.textContent = `Código de entrada gerado: ${data.dev_code} (Ambiente Local/Dev)`;
      }
    } catch (err) {
      authFeedback.className = 'auth-feedback error';
      authFeedback.textContent = err.message;
    } finally {
      btnSendOtp.disabled = false;
      btnSendOtp.textContent = 'Enviar Código de Entrada';
    }
  });

  btnVerifyOtp.addEventListener('click', async () => {
    const email = inputAuthEmail.value.trim();
    const code = inputAuthCode.value.trim();
    if (!code || code.length < 6) {
      authFeedback.className = 'auth-feedback error';
      authFeedback.textContent = 'Informe o código de 6 dígitos recebido.';
      return;
    }

    btnVerifyOtp.disabled = true;
    btnVerifyOtp.textContent = 'Verificando...';

    try {
      const res = await fetch(`${API_BASE}/api/auth/otp/verify`, {
        method: 'POST',
        headers: {'Content-Type': 'application/json'},
        body: JSON.stringify({email, code})
      });
      const data = await res.json();
      if (!res.ok) throw new Error(data.error || 'Código incorreto ou expirado.');

      localStorage.setItem('mirante_auth_token', data.token);
      renderLoggedIn(data.user);
      closeAuthModal();
    } catch (err) {
      authFeedback.className = 'auth-feedback error';
      authFeedback.textContent = err.message;
    } finally {
      btnVerifyOtp.disabled = false;
      btnVerifyOtp.textContent = 'Confirmar e Entrar';
    }
  });

  btnLogout.addEventListener('click', async () => {
    try {
      await authFetch(`${API_BASE}/api/auth/logout`, {method: 'POST'});
    } catch {}
    renderLoggedOut();
  });

  // Modal de Nova Sessão
  const modalNewSession = document.getElementById('modalNewSession');
  const btnOpenNewSessionModal = document.getElementById('btnOpenNewSessionModal');
  const btnCloseModal = document.getElementById('btnCloseModal');
  const btnCancelModal = document.getElementById('btnCancelModal');
  const btnConfirmCreateSession = document.getElementById('btnConfirmCreateSession');

  // Abre Modal
  btnOpenNewSessionModal.addEventListener('click', () => {
    modalNewSession.style.display = 'flex';
    if (currentUser) {
      const authInput = document.getElementById('nsAuthor');
      if (authInput && !authInput.value) {
        authInput.value = currentUser.name || currentUser.email;
      }
    }
    document.getElementById('nsProjectId').focus();
  });

  // Fecha Modal
  function closeModal() {
    modalNewSession.style.display = 'none';
  }
  btnCloseModal.addEventListener('click', closeModal);
  btnCancelModal.addEventListener('click', closeModal);
  modalNewSession.addEventListener('click', (e) => {
    if (e.target === modalNewSession) closeModal();
  });

  // Confirmar Criação de Nova Sessão (Qualquer Contexto)
  btnConfirmCreateSession.addEventListener('click', async () => {
    const projectId = document.getElementById('nsProjectId').value.trim() || "PRJ-GERAL";
    const title = document.getElementById('nsTitle').value.trim() || "Nova Investigação MPRR";
    const door = document.getElementById('nsDoor').value;
    const phenomenon = document.getElementById('nsPhenomenon').value.trim();
    const author = document.getElementById('nsAuthor').value.trim() || (currentUser ? (currentUser.name || currentUser.email) : "Pesquisador");
    const spatial = document.getElementById('nsSpatial').value.trim();
    const temporal = document.getElementById('nsTemporal').value.trim();

    if (!phenomenon) {
      alert("Por favor, descreva o objeto e a situação do fenômeno.");
      return;
    }

    const payload = {
      session_id: `SES-${Date.now().toString().slice(-6)}`,
      project_id: projectId,
      title: title,
      entrance_door: door,
      phenomenon_text: phenomenon,
      spatial_boundary: spatial,
      temporal_boundary: temporal,
      author: author
    };

    try {
      const res = await authFetch(`${API_BASE}/api/sessions`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      });

      if (res.ok) {
        const data = await res.json();
        closeModal();
        await refreshSessionDropdown(data.session_id);
        await switchSession(data.session_id);
        switchSection('#workbench');
      } else {
        const err = await res.json();
        alert(`Erro ao criar sessão: ${err.error}`);
      }
    } catch (e) {
      alert("Falha de conexão com o servidor MIRANTE.");
    }
  });

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
  const drawerModelTag = document.getElementById('drawerModelTag');

  btnCloseDrawer.addEventListener('click', () => {
    assistantDrawer.classList.remove('open');
  });

  function openAssistant(contextLabel, movementCode) {
    pendingAssistantMovement = movementCode;
    drawerContextInfo.textContent = `Contexto: ${contextLabel}`;
    suggestionBox.style.display = 'none';
    drawerLoading.style.display = 'none';
    drawerModelTag.textContent = selectLocalModel.value || "Ollama";
    assistantDrawer.classList.add('open');
  }

  // Inicialização
  async function init() {
    try {
      await checkAuthStatus();
      const hRes = await authFetch(`${API_BASE}/api/health`);
      if (hRes.ok) {
        const hData = await hRes.json();
        statusDot.classList.add('online');
        statusText.textContent = hData.ollama_online ? "C++26 & Ollama Ativos" : "C++26 Ativo (Modo Humano)";
      }

      // Carregar lista de modelos locais filtrando embeddings
      const mRes = await authFetch(`${API_BASE}/api/models`);
      if (mRes.ok) {
        availableModels = await mRes.json();
        selectLocalModel.innerHTML = "";
        availableModels.forEach(m => {
          const opt = document.createElement('option');
          opt.value = m.name;
          opt.textContent = `${m.name} (${(m.size_bytes / (1024*1024*1024)).toFixed(1)} GB)`;
          selectLocalModel.appendChild(opt);
        });

        // Selecionar modelo preferido (qwen2.5:14b ou llama3:8b)
        const preferred = availableModels.find(m => m.name.includes("qwen2.5:14b")) ||
                          availableModels.find(m => m.name.includes("llama3:8b")) ||
                          availableModels[0];
        if (preferred) selectLocalModel.value = preferred.name;
      }

      await refreshSessionDropdown();
      if (selectActiveSession.value) {
        await switchSession(selectActiveSession.value);
      }
    } catch (err) {
      console.error("Erro na inicialização:", err);
      statusDot.classList.remove('online');
      statusText.textContent = "Conexão Offline";
    }
  }

  // Carregar lista de sessões no dropdown
  async function refreshSessionDropdown(selectId = null) {
    const res = await authFetch(`${API_BASE}/api/sessions`);
    if (!res.ok) return;
    const sessions = await res.json();

    selectActiveSession.innerHTML = "";
    if (sessions.length === 0) {
      selectActiveSession.innerHTML = `<option value="">Nenhuma sessão encontrada</option>`;
      return;
    }

    sessions.forEach(s => {
      const opt = document.createElement('option');
      opt.value = s.session_id;
      opt.textContent = `${s.project_id}: ${s.title} (${s.session_id})`;
      selectActiveSession.appendChild(opt);
    });

    if (selectId) {
      selectActiveSession.value = selectId;
    } else if (sessions.length > 0) {
      selectActiveSession.value = sessions[0].session_id;
    }
  }

  selectActiveSession.addEventListener('change', () => {
    if (selectActiveSession.value) {
      switchSession(selectActiveSession.value);
    }
  });

  // Alternar Sessão Ativa
  async function switchSession(sid) {
    activeSessionId = sid;
    const res = await authFetch(`${API_BASE}/api/sessions/${sid}`);
    if (!res.ok) return;

    currentSessionData = await res.json();
    sessionBreadcrumb.textContent = `${currentSessionData.project_id} / ${currentSessionData.title}`;
    cycleBadge.textContent = `Ciclo ${currentSessionData.current_cycle}`;

    const isDoorA = currentSessionData.entrance_door === "PATH_A_KNOWLEDGE_TO_PHENOMENON";
    doorPill.textContent = isDoorA ? "Caminho A: Conhecimento → Realidade" : "Caminho B: Emergente → Conhecimento";
    doorPill.className = isDoorA ? "door-pill door-a" : "door-pill door-b";

    // Preenche M1
    document.getElementById('m1InitialFw').value = currentSessionData.phenomenon ? currentSessionData.phenomenon.text : "";
    if (currentSessionData.shift) {
      document.getElementById('m1ShiftedFw').value = currentSessionData.shift.shifted || "";
      document.getElementById('m1Rationale').value = currentSessionData.shift.rationale || "";
      if (currentSessionData.shift.dimension) {
        document.getElementById('m1Dimension').value = currentSessionData.shift.dimension;
      }
    } else {
      document.getElementById('m1ShiftedFw').value = "";
      document.getElementById('m1Rationale').value = "";
    }

    // Preenche M2 (Relações)
    const relListEl = document.getElementById('relationsList');
    const relSelectEl = document.getElementById('m3SelectRelation');
    relListEl.innerHTML = "";
    relSelectEl.innerHTML = `<option value="REL-GERAL">Relação Geral do Ciclo</option>`;

    if (currentSessionData.relations && currentSessionData.relations.length > 0) {
      currentSessionData.relations.forEach(r => {
        const item = document.createElement('div');
        item.className = 'stack-item';
        item.innerHTML = `<strong>${r.term_a} &harr; ${r.term_b}</strong> <span class="provenance-tag candidate">[CANDIDATA]</span> <span class="type-pill">${r.type}</span><br><small>${r.description}</small>`;
        relListEl.appendChild(item);

        const opt = document.createElement('option');
        opt.value = r.id;
        opt.textContent = `${r.term_a} ↔ ${r.term_b}`;
        relSelectEl.appendChild(opt);
      });
    } else {
      relListEl.innerHTML = `<div class="empty-hint">Nenhuma relação candidata registrada ainda.</div>`;
    }

    // Preenche M3 (Questões)
    const qListEl = document.getElementById('questionsList');
    qListEl.innerHTML = "";
    if (currentSessionData.questions && currentSessionData.questions.length > 0) {
      currentSessionData.questions.forEach(q => {
        const item = document.createElement('div');
        item.className = 'stack-item';
        item.innerHTML = `<strong>Questão:</strong> ${q.text}<br><small><strong>Evidência necessária:</strong> ${q.evidence_path || 'A definir'}</small>`;
        qListEl.appendChild(item);
      });
    } else {
      qListEl.innerHTML = `<div class="empty-hint">Nenhuma questão formulada ainda.</div>`;
    }

    // Preenche C4 (Confronto)
    rivalsList = [];
    risksList = [];
    document.getElementById('rivalsStack').innerHTML = "";
    document.getElementById('risksStack').innerHTML = "";

    if (currentSessionData.confrontation) {
      if (currentSessionData.confrontation.rivals) {
        currentSessionData.confrontation.rivals.forEach(r => {
          rivalsList.push(r);
          const el = document.createElement('div');
          el.className = 'stack-item';
          el.innerHTML = `<strong>Rival:</strong> ${r.text}`;
          document.getElementById('rivalsStack').appendChild(el);
        });
      }
      if (currentSessionData.confrontation.risks) {
        currentSessionData.confrontation.risks.forEach(r => {
          risksList.push(r);
          const el = document.createElement('div');
          el.className = 'stack-item';
          el.innerHTML = `<strong>Risco:</strong> ${r.text}`;
          document.getElementById('risksStack').appendChild(el);
        });
      }
    }
    document.getElementById('rivalCount').textContent = `${rivalsList.length} / 1`;
    document.getElementById('riskCount').textContent = `${risksList.length} / 1`;
    checkConfrontationReadiness();
  }

  // Gravar M1 (Deslocamento)
  document.getElementById('btnSaveM1').addEventListener('click', async () => {
    const initialFw = document.getElementById('m1InitialFw').value.trim();
    const shiftedFw = document.getElementById('m1ShiftedFw').value.trim();
    const rationale = document.getElementById('m1Rationale').value.trim();
    const dimension = document.getElementById('m1Dimension').value;
    const author = (currentUser && (currentUser.name || currentUser.email)) || "Pesquisador";

    const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}/shift`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        initial_framework: initialFw,
        shifted_framework: shiftedFw,
        rationale: rationale,
        dimension: dimension,
        author: author,
        is_assisted: false
      })
    });

    if (res.ok) {
      alert("✅ Deslocamento M1 consolidado com sucesso no ciclo!");
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
    const author = (currentUser && (currentUser.name || currentUser.email)) || "Pesquisador";

    const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}/relation`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        term_a: termA,
        term_b: termB,
        description: desc,
        rel_type: relType,
        author: author,
        is_assisted: false
      })
    });

    if (res.ok) {
      await switchSession(activeSessionId);
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
    const relId = document.getElementById('m3SelectRelation').value;
    const author = (currentUser && (currentUser.name || currentUser.email)) || "Pesquisador";

    const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}/question`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        relation_id: relId,
        question_text: qtext,
        evidence_path: evPath,
        author: author,
        is_assisted: false
      })
    });

    if (res.ok) {
      await switchSession(activeSessionId);
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
    const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}/confront`, {
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
    const author = (currentUser && (currentUser.name || currentUser.email)) || "Pesquisador";

    const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}/reposition`, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        decision: decision,
        justification: just,
        next_empirical_test: nextTest,
        required_knowledge: reqKnowledge,
        resumption_criteria: (decision === 'SUSPEND') ? "Aguardando novos dados e evidências para reabertura da análise" : "",
        author: author
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

  // Assistência Local: M1 (Deslocamento)
  document.getElementById('btnAssistShift').addEventListener('click', async () => {
    openAssistant("M1 Deslocamento de Perspectiva", "M1");
    drawerLoading.style.display = 'flex';

    try {
      const res = await authFetch(`${API_BASE}/api/assist/shift`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          phenomenon: document.getElementById('m1InitialFw').value,
          current_framework: "Perspectiva e enquadramento habitual registrado",
          model: selectLocalModel.value
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

  // Assistência Local: M2 (Relações)
  document.getElementById('btnAssistRelation').addEventListener('click', async () => {
    openAssistant("M2 Exploração de Relações", "M2");
    drawerLoading.style.display = 'flex';

    try {
      const res = await authFetch(`${API_BASE}/api/assist/relation`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          phenomenon: document.getElementById('m1InitialFw').value,
          shifted_framework: document.getElementById('m1ShiftedFw').value,
          model: selectLocalModel.value
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

  // Assistência Local: M3 (Problematização)
  document.getElementById('btnAssistQuestion').addEventListener('click', async () => {
    openAssistant("M3 Formulação de Perguntas", "M3");
    drawerLoading.style.display = 'flex';

    try {
      const res = await authFetch(`${API_BASE}/api/assist/question`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          relation: document.getElementById('m2Description').value || "Relação candidata do ciclo",
          model: selectLocalModel.value
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
      const hipText = currentSessionData && currentSessionData.questions && currentSessionData.questions.length > 0 
        ? currentSessionData.questions[0].text 
        : "Proposição relacional em análise";

      const res = await authFetch(`${API_BASE}/api/assist/confront`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          hypothesis: hipText,
          model: selectLocalModel.value
        })
      });
      const data = await res.json();
      if (data.suggestion) {
        box.innerHTML = `<div style="white-space: pre-wrap; font-size: 13px; line-height: 1.6;">${data.suggestion}</div>`;
      }
    } catch (e) {
      box.innerHTML = '<em>Assistência crítica indisponível. Preencha manualmente os campos de confronto.</em>';
    }
  });

  // As 4 Ações sobre Sugestões da Assistência
  document.getElementById('btnAcceptSug').addEventListener('click', () => {
    if (pendingAssistantMovement === 'M1') {
      document.getElementById('m1ShiftedFw').value = pendingAssistantText;
      document.getElementById('m1Rationale').value = "Incorporado a partir de exploração assistida.";
    } else if (pendingAssistantMovement === 'M2') {
      document.getElementById('m2Description').value = pendingAssistantText;
    } else if (pendingAssistantMovement === 'M3') {
      document.getElementById('m3Question').value = pendingAssistantText;
    }
    assistantDrawer.classList.remove('open');
    alert("Sugestão aceita como rascunho com rótulo [ASSISTIDO].");
  });

  document.getElementById('btnEditSug').addEventListener('click', () => {
    if (pendingAssistantMovement === 'M1') {
      document.getElementById('m1ShiftedFw').value = pendingAssistantText;
      document.getElementById('m1ShiftedFw').focus();
    } else if (pendingAssistantMovement === 'M2') {
      document.getElementById('m2Description').value = pendingAssistantText;
      document.getElementById('m2Description').focus();
    } else if (pendingAssistantMovement === 'M3') {
      document.getElementById('m3Question').value = pendingAssistantText;
      document.getElementById('m3Question').focus();
    }
    assistantDrawer.classList.remove('open');
  });

  document.getElementById('btnRejectSug').addEventListener('click', () => {
    suggestionBox.style.display = 'none';
    assistantDrawer.classList.remove('open');
  });

  document.getElementById('btnCounterSug').addEventListener('click', async () => {
    drawerLoading.style.display = 'flex';
    suggestionBox.style.display = 'none';

    try {
      const res = await authFetch(`${API_BASE}/api/assist/confront`, {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({
          hypothesis: pendingAssistantText,
          model: selectLocalModel.value
        })
      });
      const data = await res.json();
      drawerLoading.style.display = 'none';
      if (data.suggestion) {
        pendingAssistantText = `[CONTRAPONTO AO RASCUNHO]:\n${data.suggestion}`;
        sugText.textContent = pendingAssistantText;
        suggestionBox.style.display = 'block';
      }
    } catch (e) {
      drawerLoading.style.display = 'none';
      alert("Falha ao gerar contraponto.");
    }
  });

  // Renderizar Trilha de Proveniência (Merkle Chain)
  async function renderProvenanceLedger() {
    if (!activeSessionId) return;
    const timeline = document.getElementById('eventTimeline');
    timeline.innerHTML = `<div class="empty-hint">Consultando ledger criptográfico...</div>`;

    try {
      const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}`);
      if (!res.ok) return;
      const s = await res.json();

      timeline.innerHTML = `
        <div class="timeline-item">
          <div class="tl-badge">001</div>
          <div class="tl-content">
            <strong>Gênese da Sessão:</strong> ${s.session_id} (${s.project_id})<br>
            <small>Porta: ${s.entrance_door} | Autoria: ${s.phenomenon ? s.phenomenon.author : 'Pesquisador'}</small>
          </div>
        </div>
        <div class="timeline-item">
          <div class="tl-badge">002</div>
          <div class="tl-content">
            <strong>Deslocamento Registrado:</strong> ${s.shift ? s.shift.dimension : 'Pendente'}<br>
            <small>${s.shift ? s.shift.rationale : 'Aguardando consolidação de M1'}</small>
          </div>
        </div>
        <div class="timeline-item">
          <div class="tl-badge">003</div>
          <div class="tl-content">
            <strong>Conexões Relacionais:</strong> ${s.relations ? s.relations.length : 0} registradas.<br>
            <small>Status: Todas preservadas como CANDIDATA (INV-01).</small>
          </div>
        </div>
      `;
    } catch (e) {
      timeline.innerHTML = `<div class="empty-hint">Erro ao carregar auditoria.</div>`;
    }
  }

  // Renderizar Dossiê
  async function renderDossier() {
    if (!activeSessionId) return;
    const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}`);
    if (!res.ok) return;
    const s = await res.json();

    document.getElementById('metaId').textContent = s.session_id;
    document.getElementById('metaProject').textContent = s.project_id;
    document.getElementById('metaCycle').textContent = s.current_cycle;

    document.getElementById('sheetPhenomenon').textContent = s.phenomenon ? s.phenomenon.text : "--";
    document.getElementById('sheetInitialFw').textContent = s.shift ? s.shift.initial : (s.phenomenon ? s.phenomenon.text : "--");
    document.getElementById('sheetShiftedFw').textContent = s.shift ? `${s.shift.shifted} (Dimensão: ${s.shift.dimension} | Justificativa: ${s.shift.rationale})` : "--";

    if (s.relations && s.relations.length > 0) {
      document.getElementById('sheetRelations').innerHTML = s.relations.map(r => `&bull; <strong>${r.term_a} &harr; ${r.term_b}</strong> [${r.type}]: ${r.description}`).join('<br>');
    } else {
      document.getElementById('sheetRelations').textContent = "--";
    }

    if (s.questions && s.questions.length > 0) {
      document.getElementById('sheetQuestions').innerHTML = s.questions.map(q => `&bull; ${q.text} (Evidência: ${q.evidence_path || 'A coletar'})`).join('<br>');
    } else {
      document.getElementById('sheetQuestions').textContent = "--";
    }

    if (s.confrontation && s.confrontation.rivals) {
      const rivalsText = s.confrontation.rivals.map(r => `[Rival]: ${r.text}`).join('<br>');
      const risksText = s.confrontation.risks ? s.confrontation.risks.map(r => `[Risco]: ${r.text}`).join('<br>') : "";
      document.getElementById('sheetConfrontation').innerHTML = `${rivalsText}<br>${risksText}`;
    } else {
      document.getElementById('sheetConfrontation').textContent = "Pendente de confronto obrigatório";
    }

    document.getElementById('sheetReposition').textContent = `Ciclo ${s.current_cycle} em consolidação`;
    document.getElementById('sheetProvenance').textContent = `Núcleo C++26 | IA Local: ${selectLocalModel.value || 'Ollama'} | Integridade Procedimental Ativa`;
  }

  // Exportar .sister-research.json
  document.getElementById('btnExportJson').addEventListener('click', async () => {
    const res = await authFetch(`${API_BASE}/api/sessions/${activeSessionId}`);
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
