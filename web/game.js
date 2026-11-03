/**
 * SPACE JAM: The Last Vector — HTML5 Web App Engine
 * Created by Harper Kollins
 */

(function () {
  'use strict';

  const CANVAS = document.getElementById('game-canvas');
  const CTX = CANVAS.getContext('2d');
  const W = 1280;
  const H = 720;

  // ── Audio Engine ─────────────────────────────────────────────────────────────
  class AudioManager {
    constructor() {
      this.enabled = true;
      this.ctx = null;
      this.bgMusic = null;
      this.initContext();
    }

    initContext() {
      const AudioContext = window.AudioContext || window.webkitAudioContext;
      if (AudioContext) {
        this.ctx = new AudioContext();
      }
    }

    playTone(freq, type = 'sawtooth', duration = 0.1, gain = 0.15) {
      if (!this.enabled || !this.ctx) return;
      if (this.ctx.state === 'suspended') this.ctx.resume();
      try {
        const osc = this.ctx.createOscillator();
        const g = this.ctx.createGain();
        osc.type = type;
        osc.frequency.setValueAtTime(freq, this.ctx.currentTime);
        g.gain.setValueAtTime(gain, this.ctx.currentTime);
        g.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + duration);
        osc.connect(g);
        g.connect(this.ctx.destination);
        osc.start();
        osc.stop(this.ctx.currentTime + duration);
      } catch (e) {}
    }

    playLaser() {
      if (!this.enabled || !this.ctx) return;
      if (this.ctx.state === 'suspended') this.ctx.resume();
      try {
        const osc = this.ctx.createOscillator();
        const g = this.ctx.createGain();
        osc.type = 'sawtooth';
        osc.frequency.setValueAtTime(880, this.ctx.currentTime);
        osc.frequency.exponentialRampToValueAtTime(110, this.ctx.currentTime + 0.12);
        g.gain.setValueAtTime(0.18, this.ctx.currentTime);
        g.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + 0.12);
        osc.connect(g);
        g.connect(this.ctx.destination);
        osc.start();
        osc.stop(this.ctx.currentTime + 0.12);
      } catch (e) {}
    }

    playExplosion() {
      if (!this.enabled || !this.ctx) return;
      if (this.ctx.state === 'suspended') this.ctx.resume();
      try {
        const buf = this.ctx.createBuffer(1, this.ctx.sampleRate * 0.4, this.ctx.sampleRate);
        const data = buf.getChannelData(0);
        for (let i = 0; i < data.length; i++) data[i] = Math.random() * 2 - 1;
        const noise = this.ctx.createBufferSource();
        noise.buffer = buf;
        const filter = this.ctx.createBiquadFilter();
        filter.type = 'lowpass';
        filter.frequency.setValueAtTime(450, this.ctx.currentTime);
        filter.frequency.exponentialRampToValueAtTime(40, this.ctx.currentTime + 0.4);
        const g = this.ctx.createGain();
        g.gain.setValueAtTime(0.3, this.ctx.currentTime);
        g.gain.exponentialRampToValueAtTime(0.001, this.ctx.currentTime + 0.4);
        noise.connect(filter);
        filter.connect(g);
        g.connect(this.ctx.destination);
        noise.start();
      } catch (e) {}
    }

    playChirp() { this.playTone(1200, 'sine', 0.04, 0.08); }
    playBeep()  { this.playTone(750, 'square', 0.05, 0.06); }
    playWarp()  { this.playTone(300, 'triangle', 0.4, 0.2); }
    playMedal() { this.playTone(523.25, 'triangle', 0.2, 0.2); setTimeout(() => this.playTone(659.25, 'triangle', 0.3, 0.2), 150); }
  }

  const AUDIO = new AudioManager();

  // ── Asset Preloader ──────────────────────────────────────────────────────────
  const ASSETS = {
    textures: {},
    portraits: {},
    loaded: 0,
    total: 0
  };

  function loadImg(path, category, key) {
    ASSETS.total++;
    const img = new Image();
    img.src = path;
    img.onload = () => { ASSETS[category][key] = img; ASSETS.loaded++; };
    img.onerror = () => { ASSETS.loaded++; };
  }

  // Load Textures
  loadImg('assets/textures/bg_sector1.jpg', 'textures', 'bg1');
  loadImg('assets/textures/bg_sector2.jpg', 'textures', 'bg2');
  loadImg('assets/textures/bg_sector3.jpg', 'textures', 'bg3');
  loadImg('assets/textures/bg_sector4.jpg', 'textures', 'bg4');
  loadImg('assets/textures/vanguard.png', 'textures', 'vanguard');
  loadImg('assets/textures/civilian_shuttle.png', 'textures', 'shuttle');
  loadImg('assets/textures/enemy_scout.png', 'textures', 'scout');
  loadImg('assets/textures/enemy_interceptor.png', 'textures', 'interceptor');
  loadImg('assets/textures/enemy_tank.png', 'textures', 'tank');
  loadImg('assets/textures/enemy_bomber.png', 'textures', 'bomber');
  loadImg('assets/textures/enemy_boss.png', 'textures', 'boss');

  // Load Character Portraits
  loadImg('assets/portraits/harper.jpg', 'portraits', 'harper');
  loadImg('assets/portraits/vance.jpg', 'portraits', 'vance');
  loadImg('assets/portraits/archon.jpg', 'portraits', 'archon');
  loadImg('assets/portraits/nadia.jpg', 'portraits', 'nadia');
  loadImg('assets/portraits/amara.jpg', 'portraits', 'amara');
  loadImg('assets/portraits/mira.jpg', 'portraits', 'mira');

  // ── Input State ──────────────────────────────────────────────────────────────
  const KEYS = {};
  window.addEventListener('keydown', e => {
    KEYS[e.code] = true;
    if (['Space', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight'].includes(e.code)) {
      e.preventDefault();
    }
  });
  window.addEventListener('keyup', e => { KEYS[e.code] = false; });

  // Touch control handlers
  function bindTouch(id, code) {
    const btn = document.getElementById(id);
    if (!btn) return;
    btn.addEventListener('touchstart', (e) => { e.preventDefault(); KEYS[code] = true; });
    btn.addEventListener('touchend', (e) => { e.preventDefault(); KEYS[code] = false; });
  }
  bindTouch('btn-left', 'KeyA');
  bindTouch('btn-right', 'KeyD');
  bindTouch('btn-thrust', 'KeyW');
  bindTouch('btn-fire', 'Space');

  // Top nav buttons
  document.getElementById('btn-fullscreen')?.addEventListener('click', () => {
    if (!document.fullscreenElement) {
      document.documentElement.requestFullscreen().catch(() => {});
    } else {
      document.exitFullscreen().catch(() => {});
    }
  });

  const btnAudio = document.getElementById('btn-audio');
  btnAudio?.addEventListener('click', () => {
    AUDIO.enabled = !AUDIO.enabled;
    btnAudio.textContent = AUDIO.enabled ? 'AUDIO: ON' : 'AUDIO: OFF';
  });

  // ── Particle System ──────────────────────────────────────────────────────────
  class ParticlePool {
    constructor() {
      this.particles = [];
      this.max = 350;
    }

    spawn(x, y, vx, vy, color, size, life) {
      if (this.particles.length >= this.max) this.particles.shift();
      this.particles.push({ x, y, vx, vy, color, size, life, maxLife: life });
    }

    spawnExplosion(x, y, color = '#ff4422', count = 24, speed = 160) {
      for (let i = 0; i < count; i++) {
        const angle = Math.random() * Math.PI * 2;
        const spd = (0.2 + Math.random() * 0.8) * speed;
        this.spawn(
          x, y,
          Math.cos(angle) * spd, Math.sin(angle) * spd,
          color,
          2 + Math.random() * 4,
          0.3 + Math.random() * 0.5
        );
      }
    }

    update(dt) {
      for (let i = this.particles.length - 1; i >= 0; i--) {
        const p = this.particles[i];
        p.x += p.vx * dt;
        p.y += p.vy * dt;
        p.life -= dt;
        if (p.life <= 0) this.particles.splice(i, 1);
      }
    }

    draw(ctx) {
      ctx.save();
      for (const p of this.particles) {
        const alpha = Math.max(0, p.life / p.maxLife);
        ctx.globalAlpha = alpha;
        ctx.fillStyle = p.color;
        ctx.fillRect(p.x - p.size * 0.5, p.y - p.size * 0.5, p.size, p.size);
      }
      ctx.restore();
    }
  }

  // ── Player Spacecraft ────────────────────────────────────────────────────────
  class Player {
    constructor() {
      this.init();
    }

    init() {
      this.x = W * 0.5;
      this.y = H * 0.72;
      this.vx = 0;
      this.vy = 0;
      this.rotation = -Math.PI / 2; // Pointing upwards
      this.health = 100;
      this.maxHealth = 100;
      this.shield = 0;
      this.maxShield = 0;
      this.armorReduction = 0;
      this.plasmaDamage = 12;
      this.bonusSpeed = 0;
      this.lives = 3;
      this.dead = false;
      this.iFrames = 0;
      this.weapon = 'single'; // single, double, spread, rapid
      this.fireTimer = 0;
      this.torpedoTimer = 0;
      this.shieldTimer = 0;
      this.invincibleTimer = 0;
      this.boostAnim = 0;
      this.flashTimer = 0;
    }

    update(dt) {
      if (this.dead) return;

      if (this.iFrames > 0) this.iFrames -= dt;
      if (this.flashTimer > 0) this.flashTimer -= dt;
      if (this.shieldTimer > 0) this.shieldTimer -= dt;
      if (this.invincibleTimer > 0) this.invincibleTimer -= dt;
      if (this.fireTimer > 0) this.fireTimer -= dt;

      // Rotation
      const rotSpeed = 3.6;
      if (KEYS['KeyA'] || KEYS['ArrowLeft'])  this.rotation -= rotSpeed * dt;
      if (KEYS['KeyD'] || KEYS['ArrowRight']) this.rotation += rotSpeed * dt;

      // Thrust & Hyper-Nitro
      const isThrust = (KEYS['KeyW'] || KEYS['ArrowUp']);
      const isNitro = (KEYS['ShiftLeft'] || KEYS['ShiftRight']);
      const accel = (isNitro ? 560 : 380) + (this.bonusSpeed || 0) * 0.8;
      const maxSpd = (isNitro ? 460 : 320) + (this.bonusSpeed || 0);
      if (this.torpedoTimer > 0) this.torpedoTimer -= dt;

      if (isThrust) {
        this.vx += Math.cos(this.rotation) * accel * dt;
        this.vy += Math.sin(this.rotation) * accel * dt;
        this.boostAnim = isNitro ? 1.0 : 0.6;
      } else {
        this.boostAnim *= 0.92;
      }

      if (KEYS['KeyS'] || KEYS['ArrowDown']) {
        this.vx *= 0.94;
        this.vy *= 0.94;
      }

      // Drag & speed clamp
      const spd = Math.hypot(this.vx, this.vy);
      if (spd > maxSpd) {
        this.vx = (this.vx / spd) * maxSpd;
        this.vy = (this.vy / spd) * maxSpd;
      }
      this.vx *= 0.985;
      this.vy *= 0.985;

      this.x += this.vx * dt;
      this.y += this.vy * dt;

      // Screen wrapping
      if (this.x < 0) this.x += W;
      if (this.x > W) this.x -= W;
      if (this.y < 0) this.y += H;
      if (this.y > H) this.y -= H;
    }

    draw(ctx, particles) {
      if (this.dead) return;
      if (this.iFrames > 0 && Math.floor(this.iFrames * 10) % 2 === 0) return;

      ctx.save();
      ctx.translate(this.x, this.y);
      ctx.rotate(this.rotation + Math.PI / 2); // Rotate to sprite orientation

      // 1. Engine Plasma Flame (Flexing Mach Diamonds)
      if (this.boostAnim > 0.1) {
        const flameLen = 22 + this.boostAnim * 28;
        const grad = ctx.createLinearGradient(0, 20, 0, 20 + flameLen);
        grad.addColorStop(0, '#ffffff');
        grad.addColorStop(0.3, '#00ffff');
        grad.addColorStop(1, 'rgba(0, 100, 255, 0)');
        ctx.fillStyle = grad;
        ctx.beginPath();
        ctx.moveTo(-10, 22);
        ctx.lineTo(0, 22 + flameLen);
        ctx.lineTo(10, 22);
        ctx.fill();

        // Particles
        particles.spawn(
          this.x - Math.cos(this.rotation) * 26,
          this.y - Math.sin(this.rotation) * 26,
          -Math.cos(this.rotation) * 140 + (Math.random() - 0.5) * 50,
          -Math.sin(this.rotation) * 140 + (Math.random() - 0.5) * 50,
          '#00ffff', 2 + Math.random() * 3, 0.2
        );
      }

      // 2. HK-01 Vanguard Sprite
      const sprite = ASSETS.textures['vanguard'];
      if (sprite && sprite.complete) {
        const size = 68;
        ctx.drawImage(sprite, -size / 2, -size / 2, size, size);
      } else {
        // Geometric fallback
        ctx.fillStyle = '#f0f4fc';
        ctx.beginPath();
        ctx.moveTo(0, -26);
        ctx.lineTo(18, 16);
        ctx.lineTo(0, 10);
        ctx.lineTo(-18, 16);
        ctx.closePath();
        ctx.fill();
      }

      // 3. Kinetic Shield Bubble
      if (this.shieldTimer > 0) {
        ctx.strokeStyle = '#00e5ff';
        ctx.lineWidth = 3;
        ctx.beginPath();
        ctx.arc(0, 0, 36, 0, Math.PI * 2);
        ctx.stroke();
      }

      ctx.restore();
    }
  }

  // ── Main Game Loop & Coordinator ─────────────────────────────────────────────
  class Game {
    constructor() {
      this.state = 'MAIN_MENU'; // MAIN_MENU, COMMS, PLAYING, PAUSED, GAME_OVER, GENESIS_CHOICE, HANGAR, CODEX
      this.score = 0;
      this.highScore = parseInt(localStorage.getItem('spacejam_hi') || '0', 10);
      this.credits = parseInt(localStorage.getItem('spacejam_credits') || '250', 10);
      try {
        this.upgrades = JSON.parse(localStorage.getItem('spacejam_upgrades') || '{"hull":0,"shield":0,"afterburner":0,"overcharge":0,"torpedoes":0}');
      } catch (e) {
        this.upgrades = { hull: 0, shield: 0, afterburner: 0, overcharge: 0, torpedoes: 0 };
      }
      this.upgradeCosts = [150, 320, 580, 950, 1500];
      this.selectedUpgrade = 0;
      this.codexTab = 0;
      this.codexItem = 0;
      this.wave = 1;
      this.sector = 0;
      this.player = new Player();
      this.applyUpgrades();
      this.particles = new ParticlePool();

      this.enemies = [];
      this.projectiles = [];
      this.asteroids = [];
      this.powerups = [];

      this.relics = {
        gravityCd: 0,
        temporalCd: 0,
        voidCd: 0,
        voidActive: 0,
        temporalActive: 0
      };

      this.stars = [];
      for (let i = 0; i < 180; i++) {
        this.stars.push({
          x: Math.random() * W,
          y: Math.random() * H,
          speed: 15 + Math.random() * 65,
          size: Math.random() * 1.8 + 0.6,
          lum: Math.random()
        });
      }

      this.boss = null;
      this.commsSeq = 'PROLOGUE';
      this.commsLine = 0;
      this.commsText = '';
      this.commsChar = 0;
      this.commsTimer = 0;

      this.initEvents();
      this.startWave(1);
    }

    startWave(w) {
      this.wave = w;
      this.sector = Math.min(3, Math.floor((w - 1) / 3));
      this.enemies = [];
      this.asteroids = [];

      // Spawn regular enemies
      const count = 5 + (w - 1) * 3;
      for (let i = 0; i < count; i++) {
        const types = ['scout', 'interceptor'];
        if (w >= 4) types.push('tank');
        if (w >= 7) types.push('bomber');
        const type = types[Math.floor(Math.random() * types.length)];
        this.enemies.push({
          x: Math.random() * W,
          y: Math.random() * (H * 0.4),
          vx: (Math.random() - 0.5) * 80,
          vy: Math.random() * 40 + 20,
          rotation: Math.PI / 2,
          type,
          health: type === 'tank' ? 80 : type === 'bomber' ? 60 : 30,
          maxHealth: type === 'tank' ? 80 : type === 'bomber' ? 60 : 30,
          fireTimer: Math.random() * 2 + 1,
          radius: type === 'tank' ? 24 : 16
        });
      }

      // Sector 1 Refugee Evacuation Shuttles
      if (this.sector === 0) {
        for (let i = 0; i < 2; i++) {
          this.enemies.push({
            x: W * 0.2 + i * 400,
            y: H * 0.85,
            vx: (Math.random() - 0.5) * 20,
            vy: -45, // Evacuating upwards towards orbit
            rotation: -Math.PI / 2,
            type: 'shuttle',
            health: 70,
            maxHealth: 70,
            radius: 20
          });
        }
      }

      // Boss on wave 3, 6, 9, 12
      if (w % 3 === 0) {
        this.boss = {
          x: W * 0.5,
          y: 120,
          vx: 80,
          vy: 0,
          rotation: Math.PI / 2,
          type: 'boss',
          health: 400 + w * 60,
          maxHealth: 400 + w * 60,
          fireTimer: 1.5,
          radius: 52
        };
        this.enemies.push(this.boss);
        AUDIO.playWarp();
      } else {
        this.boss = null;
      }

      // Spawn Asteroids
      for (let i = 0; i < 4 + w; i++) {
        this.asteroids.push({
          x: Math.random() * W,
          y: Math.random() * (H * 0.5),
          vx: (Math.random() - 0.5) * 50,
          vy: (Math.random() - 0.5) * 50,
          radius: 18 + Math.random() * 22,
          rot: Math.random() * Math.PI,
          rotSpd: (Math.random() - 0.5) * 1.5
        });
      }
    }

    initEvents() {
      window.addEventListener('keydown', e => {
        if (e.code === 'KeyQ') this.useRelic('gravity');
        if (e.code === 'KeyE') this.useRelic('temporal');
        if (e.code === 'KeyF') this.useRelic('void');

        if (this.state === 'MAIN_MENU') {
          if (e.code === 'Enter' || e.code === 'Space') {
            this.state = 'PLAYING';
            this.applyUpgrades();
            AUDIO.playWarp();
          } else if (e.code === 'KeyH') {
            this.state = 'HANGAR';
          } else if (e.code === 'KeyC') {
            this.state = 'CODEX';
          }
        } else if (this.state === 'HANGAR') {
          const upgKeys = ['hull', 'shield', 'afterburner', 'overcharge', 'torpedoes'];
          if (e.code === 'KeyW' || e.code === 'ArrowUp') {
            this.selectedUpgrade = (this.selectedUpgrade - 1 + 5) % 5;
            AUDIO.playBeep();
          } else if (e.code === 'KeyS' || e.code === 'ArrowDown') {
            this.selectedUpgrade = (this.selectedUpgrade + 1) % 5;
            AUDIO.playBeep();
          } else if (e.code === 'Enter' || e.code === 'Space') {
            this.buyUpgrade(upgKeys[this.selectedUpgrade]);
          } else if (e.code === 'Escape' || e.code === 'KeyB') {
            this.state = 'MAIN_MENU';
          }
        } else if (this.state === 'CODEX') {
          if (e.code === 'Digit1') this.codexTab = 0;
          if (e.code === 'Digit2') this.codexTab = 1;
          if (e.code === 'Digit3') this.codexTab = 2;
          if (e.code === 'KeyA' || e.code === 'ArrowLeft') this.codexItem = (this.codexItem - 1 + 6) % 6;
          if (e.code === 'KeyD' || e.code === 'ArrowRight') this.codexItem = (this.codexItem + 1) % 6;
          if (e.code === 'Escape' || e.code === 'KeyB') this.state = 'MAIN_MENU';
        } else if (this.state === 'PLAYING') {
          if (e.code === 'KeyC') this.fireTorpedo();
        } else if (this.state === 'GAME_OVER' && (e.code === 'Enter' || e.code === 'Space')) {
          this.player.init();
          this.score = 0;
          this.startWave(1);
          this.applyUpgrades();
          this.state = 'PLAYING';
        } else if (this.state === 'GENESIS_CHOICE') {
          if (e.code === 'Digit1') this.chooseEnding(0);
          if (e.code === 'Digit2') this.chooseEnding(1);
          if (e.code === 'Digit3') this.chooseEnding(2);
        }
      });
    }

    useRelic(relic) {
      if (this.state !== 'PLAYING') return;
      if (relic === 'gravity' && this.relics.gravityCd <= 0) {
        this.relics.gravityCd = 12;
        AUDIO.playWarp();
        // Gravity Singularity: Draw in and damage all hostiles
        for (const e of this.enemies) {
          if (e.type !== 'shuttle') {
            const dx = this.player.x - e.x;
            const dy = this.player.y - e.y;
            e.vx += (dx * 0.8);
            e.vy += (dy * 0.8);
            e.health -= 35;
          }
        }
        this.particles.spawnExplosion(this.player.x, this.player.y, '#9900ff', 48, 280);
      } else if (relic === 'temporal' && this.relics.temporalCd <= 0) {
        this.relics.temporalCd = 16;
        this.relics.temporalActive = 4.0;
        this.player.health = Math.min(this.player.maxHealth, this.player.health + 35);
        AUDIO.playChirp();
      } else if (relic === 'void' && this.relics.voidCd <= 0) {
        this.relics.voidCd = 14;
        this.relics.voidActive = 3.5;
        AUDIO.playWarp();
      }
    }

    chooseEnding(idx) {
      this.ending = idx;
      AUDIO.playMedal();
      setTimeout(() => { this.state = 'MAIN_MENU'; }, 4500);
    }

    update(dt) {
      this.pollGamepad();
      if (this.relics.gravityCd > 0) this.relics.gravityCd -= dt;
      if (this.relics.temporalCd > 0) this.relics.temporalCd -= dt;
      if (this.relics.voidCd > 0) this.relics.voidCd -= dt;
      if (this.relics.voidActive > 0) this.relics.voidActive -= dt;
      if (this.relics.temporalActive > 0) this.relics.temporalActive -= dt;

      const timeScale = this.relics.temporalActive > 0 ? 0.35 : 1.0;

      // Stars
      for (const s of this.stars) {
        s.y += s.speed * dt * timeScale;
        if (s.y > H) { s.y = 0; s.x = Math.random() * W; }
      }

      this.particles.update(dt);

      if (this.state !== 'PLAYING') return;

      this.player.update(dt);

      // Shooting
      if (KEYS['Space'] && this.player.fireTimer <= 0) {
        this.firePlayerWeapon();
      }

      // Projectiles update
      for (let i = this.projectiles.length - 1; i >= 0; i--) {
        const p = this.projectiles[i];
        p.x += p.vx * dt * (p.isPlayer ? 1.0 : timeScale);
        p.y += p.vy * dt * (p.isPlayer ? 1.0 : timeScale);
        p.life -= dt;
        if (p.life <= 0 || p.x < 0 || p.x > W || p.y < 0 || p.y > H) {
          this.projectiles.splice(i, 1);
        }
      }

      // Enemies update & AI
      for (let i = this.enemies.length - 1; i >= 0; i--) {
        const e = this.enemies[i];
        e.x += e.vx * dt * timeScale;
        e.y += e.vy * dt * timeScale;

        // Bounce on borders
        if (e.x < e.radius || e.x > W - e.radius) e.vx = -e.vx;

        // Boss AI sweep
        if (e.type === 'boss') {
          if (e.x < 150) e.vx = Math.abs(e.vx);
          if (e.x > W - 150) e.vx = -Math.abs(e.vx);
        }

        // Hostile shooting
        if (e.type !== 'shuttle') {
          e.fireTimer -= dt * timeScale;
          if (e.fireTimer <= 0) {
            e.fireTimer = 2.0 + Math.random() * 1.5;
            const angle = Math.atan2(this.player.y - e.y, this.player.x - e.x);
            this.projectiles.push({
              x: e.x,
              y: e.y,
              vx: Math.cos(angle) * 240,
              vy: Math.sin(angle) * 240,
              life: 2.8,
              isPlayer: false
            });
          }
        }

        // Bullet collisions
        for (let j = this.projectiles.length - 1; j >= 0; j--) {
          const p = this.projectiles[j];
          if (p.isPlayer && Math.hypot(p.x - e.x, p.y - e.y) < e.radius) {
            this.projectiles.splice(j, 1);
            e.health -= (p.damage || 25);
            if (p.isTorpedo) {
              this.particles.spawnExplosion(p.x, p.y, '#ffaa22', 16, 120);
            }
            this.particles.spawn(p.x, p.y, (Math.random() - 0.5) * 80, (Math.random() - 0.5) * 80, '#00ffff', 3, 0.2);
            if (e.health <= 0) break;
          }
        }

        // Enemy dead
        if (e.health <= 0) {
          this.enemies.splice(i, 1);
          AUDIO.playExplosion();
          this.particles.spawnExplosion(e.x, e.y, e.type === 'boss' ? '#ffaa00' : '#ff4400', 32, 220);
          this.score += e.type === 'boss' ? 2500 : 150;
          const salvage = e.type === 'boss' ? 350 : (e.type === 'tank' ? 60 : (e.type === 'bomber' ? 80 : 25));
          this.credits += salvage;
          this.saveProgress();
          if (this.score > this.highScore) {
            this.highScore = this.score;
            localStorage.setItem('spacejam_hi', this.highScore);
          }
          if (e.type === 'boss' && this.wave >= 12) {
            this.state = 'GENESIS_CHOICE';
          }
        }
      }

      // Player collision with enemy bullets
      if (this.relics.voidActive <= 0 && this.player.iFrames <= 0) {
        for (let i = this.projectiles.length - 1; i >= 0; i--) {
          const p = this.projectiles[i];
          if (!p.isPlayer && Math.hypot(p.x - this.player.x, p.y - this.player.y) < 22) {
            this.projectiles.splice(i, 1);
            if (this.player.shieldTimer > 0) {
              this.player.shieldTimer = 0;
            } else {
              this.player.health -= 20;
              this.player.flashTimer = 0.25;
              this.player.iFrames = 1.0;
            }
            AUDIO.playExplosion();
            if (this.player.health <= 0) {
              this.player.dead = true;
              this.state = 'GAME_OVER';
              AUDIO.playExplosion();
            }
          }
        }
      }

      // Next Wave check
      if (this.enemies.length === 0) {
        AUDIO.playWarp();
        this.startWave(this.wave + 1);
      }
    }

    draw() {
      CTX.clearRect(0, 0, W, H);

      // 1. Panoramic Sector Background
      const bgKeys = ['bg1', 'bg2', 'bg3', 'bg4'];
      const bg = ASSETS.textures[bgKeys[this.sector]];
      if (bg && bg.complete) {
        CTX.drawImage(bg, 0, 0, W, H);
      } else {
        CTX.fillStyle = '#060a14';
        CTX.fillRect(0, 0, W, H);
      }

      // 2. Stars
      CTX.fillStyle = '#ffffff';
      for (const s of this.stars) {
        CTX.globalAlpha = 0.4 + s.lum * 0.6;
        CTX.fillRect(s.x, s.y, s.size, s.size);
      }
      CTX.globalAlpha = 1.0;

      // 3. Projectiles
      for (const p of this.projectiles) {
        CTX.fillStyle = p.isPlayer ? '#00e5ff' : '#ff2244';
        CTX.shadowBlur = 8;
        CTX.shadowColor = CTX.fillStyle;
        CTX.beginPath();
        CTX.arc(p.x, p.y, p.isPlayer ? 3.5 : 4.5, 0, Math.PI * 2);
        CTX.fill();
        CTX.shadowBlur = 0;
      }

      // 4. Enemies & Bosses
      for (const e of this.enemies) {
        CTX.save();
        CTX.translate(e.x, e.y);
        CTX.rotate(e.rotation);

        const sprite = ASSETS.textures[e.type];
        if (sprite && sprite.complete) {
          const s = e.radius * 2.3;
          CTX.drawImage(sprite, -s / 2, -s / 2, s, s);
        } else {
          CTX.fillStyle = e.type === 'shuttle' ? '#00ffaa' : e.type === 'boss' ? '#ffaa00' : '#ff3344';
          CTX.beginPath();
          CTX.arc(0, 0, e.radius, 0, Math.PI * 2);
          CTX.fill();
        }

        // Enemy Health Bar
        if (e.health < e.maxHealth) {
          const bw = e.radius * 2;
          CTX.fillStyle = 'rgba(0,0,0,0.6)';
          CTX.fillRect(-bw / 2, -e.radius - 8, bw, 4);
          CTX.fillStyle = '#00ff66';
          CTX.fillRect(-bw / 2, -e.radius - 8, bw * (e.health / e.maxHealth), 4);
        }

        CTX.restore();
      }

      // 5. Player
      this.player.draw(CTX, this.particles);

      // 6. Particles
      this.particles.draw(CTX);

      // 7. Holographic Cockpit HUD
      if (this.state === 'PLAYING') {
        this.drawHUD();
      } else if (this.state === 'MAIN_MENU') {
        this.drawMainMenu();
      } else if (this.state === 'HANGAR') {
        this.drawHangar();
      } else if (this.state === 'CODEX') {
        this.drawCodex();
      } else if (this.state === 'GAME_OVER') {
        this.drawGameOver();
      } else if (this.state === 'GENESIS_CHOICE') {
        this.drawGenesisChoice();
      }
    }


    saveProgress() {
      localStorage.setItem('spacejam_credits', this.credits.toString());
      localStorage.setItem('spacejam_upgrades', JSON.stringify(this.upgrades));
    }

    applyUpgrades() {
      this.player.maxHealth = 100 + (this.upgrades.hull || 0) * 25;
      this.player.health = Math.min(this.player.health, this.player.maxHealth);
      this.player.maxShield = (this.upgrades.shield || 0) * 35;
      this.player.shield = this.player.maxShield;
      this.player.armorReduction = (this.upgrades.hull || 0) * 0.05;
      this.player.plasmaDamage = 12 + (this.upgrades.overcharge || 0) * 4;
      this.player.bonusSpeed = (this.upgrades.afterburner || 0) * 35;
    }

    buyUpgrade(typeKey) {
      const curLvl = this.upgrades[typeKey] || 0;
      if (curLvl >= 5) return false;
      const cost = this.upgradeCosts[curLvl];
      if (this.credits >= cost) {
        this.credits -= cost;
        this.upgrades[typeKey] = curLvl + 1;
        this.saveProgress();
        this.applyUpgrades();
        AUDIO.playMedal();
        return true;
      }
      AUDIO.playBeep();
      return false;
    }

    firePlayerWeapon() {
      if (this.player.dead) return;
      this.player.fireTimer = 0.14;
      AUDIO.playLaser();
      const pSpeed = 620;
      const dirX = Math.cos(this.player.rotation);
      const dirY = Math.sin(this.player.rotation);

      this.projectiles.push({
        x: this.player.x + dirX * 24,
        y: this.player.y + dirY * 24,
        vx: dirX * pSpeed,
        vy: dirY * pSpeed,
        life: 1.5,
        damage: this.player.plasmaDamage || 12,
        isPlayer: true
      });
    }

    fireTorpedo() {
      if ((this.upgrades.torpedoes || 0) <= 0 || this.player.torpedoTimer > 0 || this.player.dead) return;
      this.player.torpedoTimer = 2.4 - this.upgrades.torpedoes * 0.25;
      const fwdX = Math.cos(this.player.rotation);
      const fwdY = Math.sin(this.player.rotation);
      this.projectiles.push({
        x: this.player.x + fwdX * 24,
        y: this.player.y + fwdY * 24,
        vx: fwdX * 270,
        vy: fwdY * 270,
        type: 'torpedo',
        damage: 60 + this.upgrades.torpedoes * 25,
        radius: 7,
        isTorpedo: true
      });
      AUDIO.playLaser();
      this.particles.spawnExplosion(this.player.x, this.player.y, '#ff9922', 10, 45);
    }

    pollGamepad() {
      const gp = navigator.getGamepads ? navigator.getGamepads()[0] : null;
      if (!gp) return;

      if (this.state === 'PLAYING') {
        const ax = gp.axes[0] || 0;
        const ay = gp.axes[1] || 0;
        if (Math.hypot(ax, ay) > 0.22) {
          this.player.rotation = Math.atan2(ay, ax);
          const isNitro = gp.buttons[4]?.pressed || gp.buttons[6]?.pressed;
          const accel = (isNitro ? 560 : 380) + (this.player.bonusSpeed || 0) * 0.8;
          this.player.vx += Math.cos(this.player.rotation) * accel * 0.016;
          this.player.vy += Math.sin(this.player.rotation) * accel * 0.016;
          this.player.boostAnim = isNitro ? 1.0 : 0.6;
        }

        // Primary fire (Button A / Trigger RT)
        if (gp.buttons[0]?.pressed || gp.buttons[7]?.pressed) {
          if (this.player.fireTimer <= 0) {
            this.player.fireTimer = 0.16;
            this.firePlayerWeapon();
          }
        }
        // Secondary Torpedo (RB)
        if (gp.buttons[5]?.pressed) {
          this.fireTorpedo();
        }
        // Relics
        if (gp.buttons[2]?.pressed) this.useRelic('gravity');
        if (gp.buttons[3]?.pressed) this.useRelic('temporal');
        if (gp.buttons[1]?.pressed) this.useRelic('void');
      } else if (this.state === 'MAIN_MENU') {
        if (gp.buttons[0]?.pressed) { this.state = 'PLAYING'; this.applyUpgrades(); AUDIO.playWarp(); }
        if (gp.buttons[2]?.pressed) { this.state = 'HANGAR'; }
        if (gp.buttons[3]?.pressed) { this.state = 'CODEX'; }
      } else if (this.state === 'HANGAR' || this.state === 'CODEX') {
        if (gp.buttons[1]?.pressed) { this.state = 'MAIN_MENU'; }
      }
    }

    drawHUD() {
      // Top Glassmorphic Bar
      CTX.fillStyle = 'rgba(6, 12, 24, 0.85)';
      CTX.fillRect(0, 0, W, 46);
      CTX.strokeStyle = 'rgba(0, 220, 255, 0.4)';
      CTX.strokeRect(0, 0, W, 46);

      // Sector Directive
      const secTitles = ['SECTOR 1 // TERRA NOVUS: DEFEND FLEET', 'SECTOR 2 // SOLAR CORONA: PURGE SIEGE', 'SECTOR 3 // XYLOS: DESTROY BLOCKADE', 'SECTOR 4 // CITADEL: ASSAULT CORE'];
      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 13px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText(secTitles[this.sector], 28, 28);

      // Score
      CTX.fillStyle = '#ffdd44';
      CTX.font = 'bold 22px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText(`SCORE: ${this.score}`, W * 0.45, 30);
      CTX.fillStyle = '#88bbdd';
      CTX.font = '12px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText(`HI: ${this.highScore}`, W * 0.58, 29);

      // Wave
      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 16px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText(`WAVE ${this.wave}`, W - 150, 29);

      // Hull Bar
      const hFrac = Math.max(0, this.player.health / this.player.maxHealth);
      CTX.fillStyle = 'rgba(8, 16, 32, 0.85)';
      CTX.fillRect(20, H - 54, 220, 16);
      CTX.fillStyle = hFrac > 0.5 ? '#00ff88' : hFrac > 0.25 ? '#ffaa00' : '#ff2244';
      CTX.fillRect(20, H - 54, 220 * hFrac, 16);
      CTX.strokeStyle = '#00e5ff';
      CTX.strokeRect(20, H - 54, 220, 16);
      CTX.fillStyle = '#ffffff';
      CTX.font = 'bold 11px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText(`HULL ${Math.round(hFrac * 100)}%`, 26, H - 42);

      // Tactical Mini-Radar Scanner (Bottom Center)
      const rx = W * 0.5, ry = H - 54, rRad = 44;
      CTX.fillStyle = 'rgba(4, 10, 24, 0.85)';
      CTX.beginPath();
      CTX.arc(rx, ry, rRad, 0, Math.PI * 2);
      CTX.fill();
      CTX.strokeStyle = 'rgba(0, 220, 255, 0.5)';
      CTX.stroke();

      // Radar sweep
      const sweep = performance.now() * 0.003;
      CTX.strokeStyle = 'rgba(0, 255, 200, 0.7)';
      CTX.beginPath();
      CTX.moveTo(rx, ry);
      CTX.lineTo(rx + Math.cos(sweep) * rRad, ry + Math.sin(sweep) * rRad);
      CTX.stroke();

      // Blips
      const rScale = rRad / (W * 0.65);
      for (const e of this.enemies) {
        const dx = (e.x - this.player.x) * rScale;
        const dy = (e.y - this.player.y) * rScale;
        if (dx * dx + dy * dy < rRad * rRad) {
          CTX.fillStyle = e.type === 'shuttle' ? '#00ffaa' : e.type === 'boss' ? '#ffdd00' : '#ff3344';
          CTX.beginPath();
          CTX.arc(rx + dx, ry + dy, e.type === 'boss' ? 4 : 2.5, 0, Math.PI * 2);
          CTX.fill();
        }
      }

      // Relic Badges (Bottom Right)
      const relics = [
        { key: '[Q] GRAVITY', cd: this.relics.gravityCd, max: 12 },
        { key: '[E] TEMPORAL', cd: this.relics.temporalCd, max: 16 },
        { key: '[F] VOID', cd: this.relics.voidCd, max: 14 }
      ];
      relics.forEach((rel, i) => {
        const bx = W - 320 + i * 105;
        const by = H - 46;
        CTX.fillStyle = rel.cd <= 0 ? 'rgba(0, 220, 255, 0.25)' : 'rgba(20, 20, 30, 0.6)';
        CTX.fillRect(bx, by, 98, 28);
        CTX.strokeStyle = rel.cd <= 0 ? '#00ffff' : '#445566';
        CTX.strokeRect(bx, by, 98, 28);
        CTX.fillStyle = rel.cd <= 0 ? '#00ffff' : '#778899';
        CTX.font = 'bold 11px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(rel.cd <= 0 ? rel.key : `${Math.ceil(rel.cd)}s`, bx + 12, by + 18);
      });
    }

    drawMainMenu() {
      CTX.fillStyle = 'rgba(4, 6, 14, 0.85)';
      CTX.fillRect(0, 0, W, H);

      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 44px 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.textAlign = 'center';
      CTX.fillText('SPACE JAM: THE LAST VECTOR', W / 2, H * 0.38);

      CTX.fillStyle = '#ffd700';
      CTX.font = '16px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('CREATED BY HARPER KOLLINS', W / 2, H * 0.44);

      CTX.fillStyle = '#ffffff';
      CTX.font = 'bold 20px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('[ PRESS ENTER OR SPACE TO LAUNCH SORTIE ]', W / 2, H * 0.58);

      CTX.fillStyle = '#00ffcc';
      CTX.font = 'bold 16px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('[ H ] HANGAR UPGRADE WORKSHOP   |   [ C ] GALACTIC CODEX & LORE', W / 2, H * 0.65);

      CTX.fillStyle = '#88aacc';
      CTX.font = '13px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('FLIGHT: WASD/ARROWS | SPACE/A = FIRE | C/RB = TORPEDO | SHIFT/LT = NITRO | Q,E,F = RELICS', W / 2, H * 0.74);
      CTX.textAlign = 'left';
    }


    drawHangar() {
      CTX.fillStyle = 'rgba(4, 8, 18, 0.94)';
      CTX.fillRect(0, 0, W, H);

      // Header
      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 28px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.textAlign = 'center';
      CTX.fillText('HANGAR BAY & UPGRADE WORKSHOP // HARPER\'S WORKBENCH', W / 2, 54);

      CTX.fillStyle = '#ffd700';
      CTX.font = 'bold 18px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText(`SALVAGE CREDITS: ${this.credits} CR`, W / 2, 88);

      const items = [
        { key: 'hull', name: 'Reinforced Titanium Hull Plating', stat: `+25 Max HP (Current: ${100 + (this.upgrades.hull || 0) * 25} HP)`, desc: 'High-tensile alloy bulkheads reduce critical kinetic damage.' },
        { key: 'shield', name: 'Aegis Deflector Shield Generator', stat: `+35 Max Shield (Current: ${(this.upgrades.shield || 0) * 35} SHD)`, desc: 'Projective magnetic barrier absorbs incoming enemy energy bolts.' },
        { key: 'afterburner', name: 'Ion Afterburner Overdrive Injectors', stat: `+40 Max Speed (Current: ${320 + (this.upgrades.afterburner || 0) * 35} SPD)`, desc: 'Increases flight agility, boost acceleration, and dodge roll response.' },
        { key: 'overcharge', name: 'Hyper-Plasma Magnetic Accelerator', stat: `+4 Gun Damage (Current: ${12 + (this.upgrades.overcharge || 0) * 4} DMG)`, desc: 'Supercharges twin autocannons with higher velocity plasma packets.' },
        { key: 'torpedoes', name: 'Seeking Photon Torpedo Launcher', stat: (this.upgrades.torpedoes > 0 ? `Tier ${this.upgrades.torpedoes} Torpedoes [Key C / RB]` : 'LOCKED — Unlocks Secondary Guided Ordnance'), desc: 'Fires heavy homing torpedoes that detonate in devastating shockwaves.' }
      ];

      items.forEach((item, i) => {
        const iy = 120 + i * 92;
        const isSel = (this.selectedUpgrade === i);
        const lvl = this.upgrades[item.key] || 0;
        const isMax = lvl >= 5;
        const cost = isMax ? 'MAX LEVEL' : `${this.upgradeCosts[lvl]} CR`;

        CTX.fillStyle = isSel ? 'rgba(0, 180, 255, 0.22)' : 'rgba(10, 20, 36, 0.7)';
        CTX.fillRect(W * 0.12, iy, W * 0.76, 78);
        CTX.strokeStyle = isSel ? '#00ffff' : '#005577';
        CTX.lineWidth = isSel ? 2 : 1;
        CTX.strokeRect(W * 0.12, iy, W * 0.76, 78);

        CTX.textAlign = 'left';
        CTX.fillStyle = isSel ? '#ffffff' : '#ccddff';
        CTX.font = 'bold 16px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(`${item.name}`, W * 0.14, iy + 26);

        CTX.fillStyle = '#00ffcc';
        CTX.font = '13px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(item.stat, W * 0.14, iy + 48);

        CTX.fillStyle = '#88aacc';
        CTX.font = '11px 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(item.desc, W * 0.14, iy + 68);

        // Level pips
        for (let p = 0; p < 5; p++) {
          CTX.fillStyle = p < lvl ? '#ffd700' : 'rgba(255, 255, 255, 0.15)';
          CTX.fillRect(W * 0.65 + p * 16, iy + 20, 12, 12);
        }

        // Cost / status
        CTX.textAlign = 'right';
        CTX.fillStyle = isMax ? '#888888' : (this.credits >= this.upgradeCosts[lvl] ? '#ffd700' : '#ff4444');
        CTX.font = 'bold 15px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(cost, W * 0.85, iy + 30);
      });

      // Controls footer
      CTX.textAlign = 'center';
      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 14px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('[ W / S / UP / DOWN ] SELECT | [ ENTER / SPACE / GAMEPAD A ] PURCHASE | [ ESC / B ] RETURN', W / 2, H - 40);
      CTX.textAlign = 'left';
    }

    drawCodex() {
      CTX.fillStyle = 'rgba(4, 8, 16, 0.95)';
      CTX.fillRect(0, 0, W, H);

      // Tabs
      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 26px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.textAlign = 'center';
      CTX.fillText('THE GALACTIC ARCHIVES & CLASSIFIED CODEX', W / 2, 46);

      const tabs = ['[ 1 ] COSMIC LORE', '[ 2 ] CHARACTER DOSSIERS', '[ 3 ] VESSEL ARCHIVES'];
      tabs.forEach((t, i) => {
        const tx = W * 0.22 + i * 260;
        CTX.fillStyle = (this.codexTab === i) ? '#ffd700' : '#00ffff';
        CTX.font = (this.codexTab === i) ? 'bold 16px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif' : '15px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(t, tx, 82);
      });

      CTX.textAlign = 'left';

      if (this.codexTab === 0) {
        // Lore tab
        const loreEntries = [
          { title: 'THE FALL OF EARTH (2049)', text: 'The Synod assault began without declaration. Biomechanical dreadnoughts fractured Earth orbit, devastating planetary defense networks and driving humanity into the Great Exodus.' },
          { title: 'THE SEVEN VECTORS', text: 'Cosmic primordial focal points seeded throughout the galaxy by the ancient Architects. Harper Kollins discovered the first Vector in Sector 1, awakening forbidden reality-bending relics.' },
          { title: 'THE SYNOD HEGEMONY', text: 'An ancient cybernetic collective dedicated to eternal preservation through absolute subjugation. Led by High Archon Malakor, who seeks to harness the Genesis Engine.' },
          { title: 'THE HOLLOW', text: 'The existential cosmic decay that consumes dying star clusters. The Synod believes forced galactic assimilation is the sole shield against total annihilation.' }
        ];

        loreEntries.forEach((l, i) => {
          const ly = 120 + i * 115;
          CTX.fillStyle = 'rgba(8, 20, 38, 0.8)';
          CTX.fillRect(W * 0.1, ly, W * 0.8, 98);
          CTX.strokeStyle = '#00ffff';
          CTX.strokeRect(W * 0.1, ly, W * 0.8, 98);

          CTX.fillStyle = '#ffd700';
          CTX.font = 'bold 15px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
          CTX.fillText(l.title, W * 0.12, ly + 28);

          CTX.fillStyle = '#ddeeff';
          CTX.font = '13px 'Segoe UI', system-ui, -apple-system, sans-serif';
          CTX.fillText(l.text, W * 0.12, ly + 58);
        });
      } else if (this.codexTab === 1) {
        // Character dossiers
        const chars = [
          { key: 'harper', name: 'Harper Kollins', role: 'Human Pilot & Vanguard Architect', age: '24', origin: 'Earth (Nigerian City)', bio: 'Genius university engineer turned starfighter ace. Wields the HK-01 Vanguard and awakens the Seven Vectors to avenge Earth and protect the Exodus fleet.' },
          { key: 'nadia', name: 'Nadia', role: 'Fleet Navigator & Harper\'s Partner', age: '23', origin: 'Earth', bio: 'Grounds Harper\'s ambition. Instrumental in mapping deep hyperspace jump corridors through the Xylos Ice Rings under heavy Synod fire.' },
          { key: 'amara', name: 'Dr. Amara Kollins', role: 'Chief Scientist & Former Synod Defector', age: '52', origin: 'Classified', bio: 'Harper and Mira\'s mother. Kept her 30-year Synod past secret until the Genesis Signal forced her ancient knowledge to light.' },
          { key: 'mira', name: 'Mira Kollins', role: 'Sub-Systems Specialist & Tech Prodigy', age: '19', origin: 'Earth', bio: 'Harper\'s fearless younger sister. Solves impossible alien telemetry codes and repairs the Odyssey\'s hyperdrive in Sector 3.' },
          { key: 'vance', name: 'Admiral Vance', role: 'Commander, Earth Defense Remnant', age: '58', origin: 'Orbital Command', bio: 'Battle-hardened fleet veteran who orchestrates the evacuation of Terra Novus and shields the civilian transports.' },
          { key: 'archon', name: 'High Archon Malakor', role: 'Supreme Sovereign of the Synod', age: 'Unknown', origin: 'The Citadel Core', bio: 'Ruthless architect of planetary pacification. Believes free will is the seed of chaos that invites The Hollow.' }
        ];

        const char = chars[this.codexItem % chars.length];

        // Portrait on Left
        const px = W * 0.12, py = 120, pw = 240, ph = 320;
        CTX.fillStyle = 'rgba(6, 12, 24, 0.8)';
        CTX.fillRect(px, py, pw, ph);
        CTX.strokeStyle = '#00ffff';
        CTX.strokeRect(px, py, pw, ph);

        const pImg = ASSETS.portraits[char.key];
        if (pImg && pImg.complete) {
          CTX.drawImage(pImg, px + 10, py + 10, pw - 20, ph - 20);
        } else {
          CTX.fillStyle = '#00e5ff';
          CTX.font = 'bold 20px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
          CTX.fillText(char.name.toUpperCase(), px + 20, py + ph / 2);
        }

        // Dossier Info on Right
        const dx = W * 0.38, dy = 120;
        CTX.fillStyle = '#ffd700';
        CTX.font = 'bold 24px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(char.name.toUpperCase(), dx, dy + 32);

        CTX.fillStyle = '#00ffff';
        CTX.font = 'bold 15px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(`ROLE: ${char.role}`, dx, dy + 68);
        CTX.fillText(`ORIGIN: ${char.origin} | AGE: ${char.age}`, dx, dy + 96);

        CTX.fillStyle = '#ccddee';
        CTX.font = '15px 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(char.bio, dx, dy + 145);

        // Navigation hint
        CTX.fillStyle = '#ffd700';
        CTX.font = 'bold 14px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(`[ A / D / LEFT / RIGHT ] PREVIOUS / NEXT DOSSIER (${(this.codexItem % chars.length) + 1} / ${chars.length})`, dx, dy + 280);
      } else {
        // Vessels tab
        const ships = [
          { name: 'HK-01 VANGUARD STARFIGHTER', type: 'High-Agility Attack Interceptor', desc: 'Harper\'s custom-built fighter featuring dual wingtip autocannons, hyper-nitro afterburners, and Vector Relic integration.' },
          { name: 'ODYSSEY COLONY CARRIER', type: 'Super-Capital Ark Ship', desc: 'Carries the remaining 850,000 human civilians in cryostasis across deep space toward the promised sanctuary world.' },
          { name: 'SYNOD DREADNOUGHT "VINDICATOR"', type: 'Planetary Siege Battleship', desc: 'Massive monolithic capital ship armed with orbital shield arrays, Solar Thermal Lances, and gravitational singularity generators.' }
        ];

        ships.forEach((s, i) => {
          const sy = 125 + i * 130;
          CTX.fillStyle = 'rgba(8, 18, 34, 0.8)';
          CTX.fillRect(W * 0.1, sy, W * 0.8, 105);
          CTX.strokeStyle = '#00ffff';
          CTX.strokeRect(W * 0.1, sy, W * 0.8, 105);

          CTX.fillStyle = '#ffd700';
          CTX.font = 'bold 16px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
          CTX.fillText(s.name, W * 0.12, sy + 32);

          CTX.fillStyle = '#00ffff';
          CTX.font = 'bold 13px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
          CTX.fillText(`CLASS: ${s.type}`, W * 0.12, sy + 58);

          CTX.fillStyle = '#ccddee';
          CTX.font = '13px 'Segoe UI', system-ui, -apple-system, sans-serif';
          CTX.fillText(s.desc, W * 0.12, sy + 84);
        });
      }

      // Footer
      CTX.textAlign = 'center';
      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 14px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('[ 1 / 2 / 3 ] SWITCH ARCHIVES TAB | [ ESC / B ] RETURN TO COMMAND DECK', W / 2, H - 40);
      CTX.textAlign = 'left';
    }

    drawGameOver() {
      CTX.fillStyle = 'rgba(20, 4, 10, 0.9)';
      CTX.fillRect(0, 0, W, H);

      CTX.fillStyle = '#ff2244';
      CTX.font = 'bold 48px 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.textAlign = 'center';
      CTX.fillText('VANGUARD DESTROYED', W / 2, H * 0.4);

      CTX.fillStyle = '#ffffff';
      CTX.font = '22px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText(`FINAL SCORE: ${this.score}`, W / 2, H * 0.5);

      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 18px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('[ PRESS ENTER TO RETRY MISSION ]', W / 2, H * 0.64);
      CTX.textAlign = 'left';
    }

    drawGenesisChoice() {
      CTX.fillStyle = 'rgba(6, 10, 24, 0.95)';
      CTX.fillRect(0, 0, W, H);

      CTX.fillStyle = '#00ffff';
      CTX.font = 'bold 28px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.textAlign = 'center';
      CTX.fillText('THE GENESIS ENGINE // THE ARCHITECT NEXUS', W / 2, 80);

      CTX.fillStyle = '#ffffff';
      CTX.font = '14px 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('THE CITADEL HAS FALLEN. HARPER KOLLINS STANDS BEFORE THE CORE OF CONSCIOUSNESS.', W / 2, 115);

      const choices = [
        { num: '[ 1 ]', title: 'BREAK THE ENGINE — THE HUMAN FUTURE (Ending A)', desc: 'Destroy the ancient machine network. Save Earth and humanity alone.' },
        { num: '[ 2 ]', title: 'CLAIM THE NEXUS — THE ETERNAL ORDER (Ending B)', desc: 'Assume control of the Synod. Enforce absolute galactic security at the cost of free will.' },
        { num: '[ 3 ]', title: 'ASCEND TO THE VECTOR — THE LAST VECTOR (Ending C / True Ending)', desc: 'Transcend mortal form. Merge with all Seven Vectors to shield all minds from The Hollow.' }
      ];

      choices.forEach((c, i) => {
        const cy = 180 + i * 110;
        CTX.fillStyle = 'rgba(10, 20, 36, 0.8)';
        CTX.fillRect(W * 0.15, cy, W * 0.7, 85);
        CTX.strokeStyle = '#00e5ff';
        CTX.strokeRect(W * 0.15, cy, W * 0.7, 85);

        CTX.fillStyle = '#ffd700';
        CTX.font = 'bold 18px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(c.num, W * 0.19, cy + 32);

        CTX.fillStyle = '#00ffff';
        CTX.font = 'bold 16px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(c.title, W * 0.5, cy + 32);

        CTX.fillStyle = '#ccddee';
        CTX.font = '13px 'Segoe UI', system-ui, -apple-system, sans-serif';
        CTX.fillText(c.desc, W * 0.5, cy + 62);
      });

      CTX.fillStyle = '#ffaa00';
      CTX.font = 'bold 15px 'Segoe UI', system-ui, -apple-system, 'Segoe UI', system-ui, -apple-system, sans-serif';
      CTX.fillText('[ PRESS 1, 2, OR 3 TO CHOOSE YOUR DESTINY ]', W / 2, H - 70);
      CTX.textAlign = 'left';
    }
  }

  // ── Run Engine ───────────────────────────────────────────────────────────────
  const game = new Game();
  let lastTime = performance.now();

  function loop(now) {
    const dt = Math.min(0.1, (now - lastTime) / 1000);
    lastTime = now;

    game.update(dt);
    game.draw();

    requestAnimationFrame(loop);
  }

  requestAnimationFrame(loop);
})();

