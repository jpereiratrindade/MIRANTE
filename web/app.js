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

  // Elementos de Autenticação SisTer / MORFO
  const btnOpenLogin = document.getElementById('btn-open-login');
  const btnSidebarAuthAction = document.getElementById('btnSidebarAuthAction');
  const sidebarDotAuth = document.getElementById('sidebarDotAuth');
  const sidebarUserLabel = document.getElementById('sidebarUserLabel');
  const userChip = document.getElementById('user-chip');
  const userAvatar = document.getElementById('user-avatar');
  const userEmail = document.getElementById('user-email');
  const userRole = document.getElementById('user-role');
  const btnLogout = document.getElementById('btn-logout');

  // Modal Auth Overlay SisTer
  const authOverlay = document.getElementById('auth-overlay');
  const btnCloseAuth = document.getElementById('btn-close-auth');
  const authEmailForm = document.getElementById('auth-email-form');
  const authEmailInput = document.getElementById('auth-email-input');
  const btnRequestCode = document.getElementById('btn-request-code');
  const authCodeForm = document.getElementById('auth-code-form');
  const authTargetEmail = document.getElementById('auth-target-email');
  const authCodeInput = document.getElementById('auth-code-input');
  const authDevPreview = document.getElementById('auth-dev-preview');
  const authDevCodeVal = document.getElementById('auth-dev-code-val');
  const btnVerifyCode = document.getElementById('btn-verify-code');
  const btnResendCode = document.getElementById('btn-resend-code');
  const btnSwitchEmail = document.getElementById('btn-switch-email');
  const authFeedback = document.getElementById('auth-feedback');

  let resendCooldown = 0;
  let resendTimerId = null;

  function setAuthFeedback(msg, type = 'info') {
    if (!authFeedback) return;
    if (!msg) {
      authFeedback.textContent = '';
      authFeedback.className = 'auth-feedback hidden';
      return;
    }
    authFeedback.textContent = msg;
    authFeedback.className = `auth-feedback ${type}`;
    authFeedback.classList.remove('hidden');
  }

  function showAuthOverlay() {
    if (!authOverlay) return;
    authOverlay.classList.remove('hidden');
    setAuthFeedback('');
    if (!authEmailInput.value && currentUser?.email) {
      authEmailInput.value = currentUser.email;
    }
    if (authCodeForm && !authCodeForm.classList.contains('hidden')) {
      authCodeInput?.focus();
    } else {
      authEmailInput?.focus();
    }
  }

  function hideAuthOverlay() {
    if (!authOverlay) return;
    authOverlay.classList.add('hidden');
    setAuthFeedback('');
  }

  function startResendCooldown(seconds = 60) {
    resendCooldown = seconds;
    if (resendTimerId) clearInterval(resendTimerId);
    if (!btnResendCode) return;
    btnResendCode.disabled = true;
    btnResendCode.textContent = `↻ Reenviar código (${resendCooldown}s)`;
    resendTimerId = setInterval(() => {
      resendCooldown -= 1;
      if (resendCooldown <= 0) {
        clearInterval(resendTimerId);
        btnResendCode.disabled = false;
        btnResendCode.textContent = '↻ Reenviar novo código';
      } else {
        btnResendCode.textContent = `↻ Reenviar código (${resendCooldown}s)`;
      }
    }, 1000);
  }

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
    if (btnOpenLogin) btnOpenLogin.classList.add('hidden');
    if (userChip) userChip.classList.remove('hidden');
    if (userEmail) userEmail.textContent = user.email || 'pesquisador@labeco.invalid';
    if (userRole) userRole.textContent = user.role === 'admin' ? 'Coordenador SisTer' : 'Pesquisador';
    
    const initials = (user.name || user.email || 'PE').substring(0, 2).toUpperCase();
    if (userAvatar) userAvatar.textContent = initials;

    if (sidebarDotAuth) {
      sidebarDotAuth.className = 'dot-online';
    }
    if (sidebarUserLabel) {
      sidebarUserLabel.textContent = user.email.split('@')[0];
    }
    if (btnSidebarAuthAction) {
      btnSidebarAuthAction.textContent = 'Conta ⚿';
    }

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
    if (btnOpenLogin) btnOpenLogin.classList.remove('hidden');
    if (userChip) userChip.classList.add('hidden');
    if (sidebarDotAuth) {
      sidebarDotAuth.className = 'dot-anon';
    }
    if (sidebarUserLabel) {
      sidebarUserLabel.textContent = 'Acesso Restrito';
    }
    if (btnSidebarAuthAction) {
      btnSidebarAuthAction.textContent = 'Entrar ⚿';
    }
  }

  // Event Listeners de Abertura / Fechamento de Auth
  if (btnOpenLogin) btnOpenLogin.addEventListener('click', showAuthOverlay);
  if (btnSidebarAuthAction) btnSidebarAuthAction.addEventListener('click', showAuthOverlay);
  if (btnCloseAuth) btnCloseAuth.addEventListener('click', hideAuthOverlay);
  if (authOverlay) {
    authOverlay.addEventListener('click', (e) => {
      if (e.target === authOverlay) hideAuthOverlay();
    });
  }
  document.addEventListener('keydown', (e) => {
    if (e.key === 'Escape' && authOverlay && !authOverlay.classList.contains('hidden')) {
      hideAuthOverlay();
    }
  });

  // Etapa 1: Solicitar Código OTP
  if (authEmailForm) {
    authEmailForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const email = authEmailInput.value.trim();
      if (!email || !email.includes('@')) {
        setAuthFeedback('Por favor, informe um endereço de e-mail válido.', 'error');
        authEmailInput.focus();
        return;
      }

      btnRequestCode.disabled = true;
      btnRequestCode.textContent = 'Disparando código...';
      setAuthFeedback('Processando requisição segura...', 'info');

      try {
        const res = await fetch(`${API_BASE}/api/auth/otp/request`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({email})
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha ao solicitar código.');

        authTargetEmail.textContent = email;
        authEmailForm.classList.add('hidden');
        authCodeForm.classList.remove('hidden');
        authCodeInput.value = '';
        authCodeInput.focus();
        startResendCooldown(60);

        if (data.dev_code) {
          if (authDevPreview) authDevPreview.classList.remove('hidden');
          if (authDevCodeVal) authDevCodeVal.textContent = data.dev_code;
          setAuthFeedback('Código gerado no modo piloto/local.', 'success');
        } else {
          if (authDevPreview) authDevPreview.classList.add('hidden');
          setAuthFeedback('Código temporário enviado para sua caixa de entrada.', 'success');
        }
      } catch (err) {
        setAuthFeedback(err.message, 'error');
      } finally {
        btnRequestCode.disabled = false;
        btnRequestCode.textContent = 'Enviar código por e-mail →';
      }
    });
  }

  // Etapa 2: Validar Código OTP
  if (authCodeForm) {
    authCodeForm.addEventListener('submit', async (e) => {
      e.preventDefault();
      const email = authTargetEmail.textContent.trim() || authEmailInput.value.trim();
      const code = authCodeInput.value.trim();
      if (!code || code.length < 6) {
        setAuthFeedback('Informe o código numérico de 6 dígitos recebido.', 'error');
        authCodeInput.focus();
        return;
      }

      btnVerifyCode.disabled = true;
      btnVerifyCode.textContent = 'Verificando...';
      setAuthFeedback('Validando integridade do código...', 'info');

      try {
        const res = await fetch(`${API_BASE}/api/auth/otp/verify`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({email, code})
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Código incorreto, expirado ou tentativas esgotadas.');

        localStorage.setItem('mirante_auth_token', data.token);
        renderLoggedIn(data.user);
        hideAuthOverlay();
      } catch (err) {
        setAuthFeedback(err.message, 'error');
      } finally {
        btnVerifyCode.disabled = false;
        btnVerifyCode.textContent = 'Confirmar código e entrar ✓';
      }
    });
  }

  // Trocar e-mail ou reenviar
  if (btnSwitchEmail) {
    btnSwitchEmail.addEventListener('click', () => {
      authCodeForm.classList.add('hidden');
      authEmailForm.classList.remove('hidden');
      setAuthFeedback('');
      authEmailInput.focus();
    });
  }

  if (btnResendCode) {
    btnResendCode.addEventListener('click', async () => {
      if (resendCooldown > 0) return;
      const email = authTargetEmail.textContent.trim() || authEmailInput.value.trim();
      if (!email) return;

      btnResendCode.disabled = true;
      btnResendCode.textContent = 'Reenviando...';
      setAuthFeedback('Enviando novo código...', 'info');

      try {
        const res = await fetch(`${API_BASE}/api/auth/otp/request`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({email})
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha ao reenviar código.');

        startResendCooldown(60);
        if (data.dev_code) {
          if (authDevPreview) authDevPreview.classList.remove('hidden');
          if (authDevCodeVal) authDevCodeVal.textContent = data.dev_code;
          setAuthFeedback('Novo código gerado no modo piloto/local.', 'success');
        } else {
          setAuthFeedback('Novo código enviado com sucesso.', 'success');
        }
      } catch (err) {
        setAuthFeedback(err.message, 'error');
      }
    });
  }

  if (btnLogout) {
    btnLogout.addEventListener('click', async () => {
      try {
        await authFetch(`${API_BASE}/api/auth/logout`, {method: 'POST'});
      } catch {}
      renderLoggedOut();
    });
  }

  // Modal de Nova Sessão
  const modalNewSession = document.getElementById('modalNewSession');
  const btnOpenNewSessionModal = document.getElementById('btnOpenNewSessionModal');
  const btnCloseModal = document.getElementById('btnCloseModal');
  const btnCancelModal = document.getElementById('btnCancelModal');
  const btnConfirmCreateSession = document.getElementById('btnConfirmCreateSession');

  // Abre Modal
  btnOpenNewSessionModal.addEventListener('click', () => {
    modalNewSession.classList.remove('hidden');
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
    modalNewSession.classList.add('hidden');
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

  // Navegação Lateral e Alternância de Telas
  const navItems = document.querySelectorAll('.sidebar-nav .nav-item, .sidebar-menu .nav-item');
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
      const targetHash = item.getAttribute('data-target') || item.getAttribute('href');
      if (targetHash) switchSection(targetHash);
      const sidebar = document.querySelector('.sidebar');
      if (sidebar && sidebar.classList.contains('open')) sidebar.classList.remove('open');
    });
  });

  function switchSection(targetHash) {
    navItems.forEach(i => i.classList.remove('active'));
    Object.values(sections).forEach(s => {
      if (s) {
        s.classList.remove('active');
        s.classList.add('hidden');
      }
    });

    const activeNav = document.querySelector(`.nav-item[data-target="${targetHash}"], .nav-item[href="${targetHash}"]`);
    if (activeNav) activeNav.classList.add('active');

    const targetSec = sections[targetHash];
    if (targetSec) {
      targetSec.classList.add('active');
      targetSec.classList.remove('hidden');
    }

    if (targetHash === '#dossier') renderDossier();
    if (targetHash === '#provenance') renderProvenanceLedger();
  }

  // Toggle do menu mobile
  const menuBtn = document.getElementById('menuBtn');
  if (menuBtn) {
    menuBtn.addEventListener('click', () => {
      const sidebar = document.querySelector('.sidebar');
      if (sidebar) sidebar.classList.toggle('open');
    });
  }

  // Trilha de Abas M1, M2, M3
  const trackTabs = document.querySelectorAll('.track-tab, .track-tab-btn');
  const tabContents = document.querySelectorAll('.tab-content');

  trackTabs.forEach(tab => {
    tab.addEventListener('click', () => {
      const tabId = tab.dataset.tab;
      trackTabs.forEach(t => t.classList.remove('active'));
      tabContents.forEach(c => {
        c.classList.remove('active');
        c.classList.add('hidden');
      });

      tab.classList.add('active');
      const targetContent = document.getElementById(tabId);
      if (targetContent) {
        targetContent.classList.add('active');
        targetContent.classList.remove('hidden');
      }
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

  function addRivalItem(text, author = "Revisor Crítico") {
    rivalsList.push({ text: text, author: author });
    const stack = document.getElementById('rivalsStack');
    const el = document.createElement('div');
    el.className = 'stack-item pulse-highlight';
    el.innerHTML = `<strong>Rival ${rivalsList.length} (${author}):</strong> ${text}`;
    stack.appendChild(el);

    document.getElementById('c4RivalText').value = "";
    document.getElementById('rivalCount').textContent = `${rivalsList.length} / 1`;
    checkConfrontationReadiness();
  }

  function addRiskItem(text, author = "Revisor Crítico") {
    risksList.push({ text: text, author: author });
    const stack = document.getElementById('risksStack');
    const el = document.createElement('div');
    el.className = 'stack-item pulse-highlight';
    el.innerHTML = `<strong>Risco ${risksList.length} (${author}):</strong> ${text}`;
    stack.appendChild(el);

    document.getElementById('c4RiskText').value = "";
    document.getElementById('riskCount').textContent = `${risksList.length} / 1`;
    checkConfrontationReadiness();
  }

  // Confronto (C4): Adicionar Rival
  document.getElementById('btnAddRival').addEventListener('click', () => {
    const text = document.getElementById('c4RivalText').value.trim();
    if (text) addRivalItem(text, "Revisor Crítico");
  });

  // Confronto (C4): Adicionar Risco
  document.getElementById('btnAddRisk').addEventListener('click', () => {
    const text = document.getElementById('c4RiskText').value.trim();
    if (text) addRiskItem(text, "Revisor Crítico");
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

  // Helper para destacar campos preenchidos diretamente
  function highlightField(el) {
    if (!el) return;
    el.classList.remove('pulse-highlight');
    void el.offsetWidth;
    el.classList.add('pulse-highlight');
  }

  // ----------------------------------------------------------------------
  // PREENCHIMENTO DIRETO COM LLM LOCAL (M1, M2, M3, C4, C5, SESSÃO)
  // ----------------------------------------------------------------------

  // 1. Preenchimento Direto M1
  const btnDirectFillM1 = document.getElementById('btnDirectFillM1');
  if (btnDirectFillM1) {
    btnDirectFillM1.addEventListener('click', async () => {
      const origText = btnDirectFillM1.textContent;
      btnDirectFillM1.classList.add('loading');
      btnDirectFillM1.textContent = '⚡ Consultando LLM...';

      const initialFw = document.getElementById('m1InitialFw').value.trim();
      const phen = currentSessionData?.phenomenon?.text || initialFw || "Fenômeno e dinâmicas sistêmicas sob investigação";

      try {
        const res = await authFetch(`${API_BASE}/api/assist/fill/m1`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            phenomenon: phen,
            current_framework: initialFw,
            model: selectLocalModel.value
          })
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha na resposta do LLM.');

        if (data.initial_framework && !initialFw) {
          const el = document.getElementById('m1InitialFw');
          el.value = data.initial_framework;
          highlightField(el);
        }
        if (data.shifted_framework) {
          const el = document.getElementById('m1ShiftedFw');
          el.value = data.shifted_framework;
          highlightField(el);
        }
        if (data.dimension) {
          const el = document.getElementById('m1Dimension');
          el.value = data.dimension;
          highlightField(el);
        }
        if (data.rationale) {
          const el = document.getElementById('m1Rationale');
          el.value = `[ASSISTIDO: ${data.model || selectLocalModel.value}] ${data.rationale}`;
          highlightField(el);
        }
      } catch (err) {
        alert("Erro no preenchimento direto via LLM: " + err.message);
      } finally {
        btnDirectFillM1.classList.remove('loading');
        btnDirectFillM1.textContent = origText;
      }
    });
  }

  // 2. Preenchimento Direto M2
  const btnDirectFillM2 = document.getElementById('btnDirectFillM2');
  if (btnDirectFillM2) {
    btnDirectFillM2.addEventListener('click', async () => {
      const origText = btnDirectFillM2.textContent;
      btnDirectFillM2.classList.add('loading');
      btnDirectFillM2.textContent = '⚡ Consultando LLM...';

      const phen = currentSessionData?.phenomenon?.text || document.getElementById('m1InitialFw').value.trim() || "Fenômeno em estudo";
      const shifted = document.getElementById('m1ShiftedFw').value.trim();

      try {
        const res = await authFetch(`${API_BASE}/api/assist/fill/m2`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            phenomenon: phen,
            shifted_framework: shifted,
            model: selectLocalModel.value
          })
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha na resposta do LLM.');

        if (data.term_a) {
          const el = document.getElementById('m2TermA');
          el.value = data.term_a;
          highlightField(el);
        }
        if (data.term_b) {
          const el = document.getElementById('m2TermB');
          el.value = data.term_b;
          highlightField(el);
        }
        if (data.rel_type) {
          const el = document.getElementById('m2RelType');
          el.value = data.rel_type;
          highlightField(el);
        }
        if (data.description) {
          const el = document.getElementById('m2Description');
          el.value = `[ASSISTIDO: ${data.model || selectLocalModel.value}] ${data.description}`;
          highlightField(el);
        }
      } catch (err) {
        alert("Erro no preenchimento direto via LLM: " + err.message);
      } finally {
        btnDirectFillM2.classList.remove('loading');
        btnDirectFillM2.textContent = origText;
      }
    });
  }

  // 3. Preenchimento Direto M3
  const btnDirectFillM3 = document.getElementById('btnDirectFillM3');
  if (btnDirectFillM3) {
    btnDirectFillM3.addEventListener('click', async () => {
      const origText = btnDirectFillM3.textContent;
      btnDirectFillM3.classList.add('loading');
      btnDirectFillM3.textContent = '⚡ Consultando LLM...';

      const rel = document.getElementById('m2Description').value.trim() || "Relação candidata do ciclo";

      try {
        const res = await authFetch(`${API_BASE}/api/assist/fill/m3`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            relation: rel,
            model: selectLocalModel.value
          })
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha na resposta do LLM.');

        if (data.question) {
          const el = document.getElementById('m3Question');
          el.value = data.question;
          highlightField(el);
        }
        if (data.evidence_hint) {
          const el = document.getElementById('m3Evidence');
          el.value = data.evidence_hint;
          highlightField(el);
        }
      } catch (err) {
        alert("Erro no preenchimento direto via LLM: " + err.message);
      } finally {
        btnDirectFillM3.classList.remove('loading');
        btnDirectFillM3.textContent = origText;
      }
    });
  }

  // 4. Preenchimento Direto C4 (Arena de Confronto)
  const btnDirectFillC4 = document.getElementById('btnDirectFillC4');
  if (btnDirectFillC4) {
    btnDirectFillC4.addEventListener('click', async () => {
      const origText = btnDirectFillC4.textContent;
      btnDirectFillC4.classList.add('loading');
      btnDirectFillC4.textContent = '⚡ Gerando Rivais e Riscos...';

      const hipText = (currentSessionData?.questions && currentSessionData.questions.length > 0)
        ? currentSessionData.questions[0].text
        : (document.getElementById('m3Question').value.trim() || "Proposição relacional em análise");

      try {
        const res = await authFetch(`${API_BASE}/api/assist/fill/confront`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            hypothesis: hipText,
            model: selectLocalModel.value
          })
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha na resposta do LLM.');

        if (data.rival_hypothesis) {
          addRivalItem(data.rival_hypothesis, data.rival_author || `Ollama [${selectLocalModel.value}]`);
        }

        if (data.risk_description) {
          const riskEntry = `[${data.risk_type || 'RISCO'}] ${data.risk_description}` + 
            (data.risk_mitigation ? ` (Mitigação: ${data.risk_mitigation})` : '');
          addRiskItem(riskEntry, `Ollama [${selectLocalModel.value}]`);
        }
      } catch (err) {
        alert("Erro no preenchimento da Arena via LLM: " + err.message);
      } finally {
        btnDirectFillC4.classList.remove('loading');
        btnDirectFillC4.textContent = origText;
      }
    });
  }

  // 5. Preenchimento Direto C5 (Reposicionamento)
  const btnDirectFillC5 = document.getElementById('btnDirectFillC5');
  if (btnDirectFillC5) {
    btnDirectFillC5.addEventListener('click', async () => {
      const origText = btnDirectFillC5.textContent;
      btnDirectFillC5.classList.add('loading');
      btnDirectFillC5.textContent = '⚡ Sintetizando Reposicionamento...';

      const hipText = (currentSessionData?.questions && currentSessionData.questions.length > 0)
        ? currentSessionData.questions[0].text
        : (document.getElementById('m3Question').value.trim() || "Proposição relacional sob escrutínio");

      const rivalsSummary = rivalsList.map(r => r.text).join('; ') || "Nenhuma rival adicional registrada";
      const risksSummary = risksList.map(r => r.text).join('; ') || "Nenhum risco adicional apontado";

      try {
        const res = await authFetch(`${API_BASE}/api/assist/fill/reposition`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            hypothesis: hipText,
            rivals_summary: rivalsSummary,
            risks_summary: risksSummary,
            model: selectLocalModel.value
          })
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha na resposta do LLM.');

        // Marca radio de decisão
        if (data.decision) {
          let radioVal = "REINFORCE";
          if (data.decision === "ABANDON" || data.decision === "REJECT") radioVal = "ABANDON";
          else if (data.decision === "SUSPEND" || data.decision === "REFORMULATE") radioVal = "SUSPEND";
          const radio = document.querySelector(`input[name="repositionDecision"][value="${radioVal}"]`);
          if (radio) {
            radio.checked = true;
            highlightField(radio.parentElement);
          }
        }

        if (data.justification) {
          const el = document.getElementById('c5Justification');
          el.value = `[ASSISTIDO: ${data.model || selectLocalModel.value}] ${data.justification}`;
          highlightField(el);
        }
        if (data.next_step) {
          const el = document.getElementById('c5NextTest');
          el.value = data.next_step;
          highlightField(el);
        }
        if (data.required_knowledge) {
          const el = document.getElementById('c5ReqKnowledge');
          el.value = data.required_knowledge;
          highlightField(el);
        }
      } catch (err) {
        alert("Erro no preenchimento de Reposicionamento via LLM: " + err.message);
      } finally {
        btnDirectFillC5.classList.remove('loading');
        btnDirectFillC5.textContent = origText;
      }
    });
  }

  // 6. Preenchimento Direto na Nova Sessão (Modal)
  const btnDirectFillSession = document.getElementById('btnDirectFillSession');
  if (btnDirectFillSession) {
    btnDirectFillSession.addEventListener('click', async () => {
      const origText = btnDirectFillSession.textContent;
      btnDirectFillSession.classList.add('loading');
      btnDirectFillSession.textContent = '⚡ Consultando LLM...';

      const promptHint = document.getElementById('nsPromptHint')?.value.trim() || "";

      try {
        const res = await authFetch(`${API_BASE}/api/assist/fill/session`, {
          method: 'POST',
          headers: {'Content-Type': 'application/json'},
          body: JSON.stringify({
            topic: promptHint,
            model: selectLocalModel.value
          })
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.error || 'Falha na resposta do LLM.');

        if (data.project_id) {
          const el = document.getElementById('nsProjectId');
          el.value = data.project_id;
          highlightField(el);
        }
        if (data.title) {
          const el = document.getElementById('nsTitle');
          el.value = data.title;
          highlightField(el);
        }
        if (data.entrance_door) {
          const el = document.getElementById('nsDoor');
          el.value = data.entrance_door === 'PORTA_A' ? 'PATH_A_KNOWLEDGE_TO_PHENOMENON' : 'PATH_B_PHENOMENON_TO_KNOWLEDGE';
          highlightField(el);
        }
        if (data.phenomenon) {
          const el = document.getElementById('nsPhenomenon');
          el.value = data.phenomenon;
          highlightField(el);
        }
        if (data.spatial) {
          const el = document.getElementById('nsSpatial');
          el.value = data.spatial;
          highlightField(el);
        }
        if (data.temporal) {
          const el = document.getElementById('nsTemporal');
          el.value = data.temporal;
          highlightField(el);
        }
      } catch (err) {
        alert("Erro ao preencher com LLM: " + err.message);
      } finally {
        btnDirectFillSession.classList.remove('loading');
        btnDirectFillSession.textContent = origText;
      }
    });
  }

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
