# V6R TypeScript Integration Guide

## Installation

```bash
# Install the generated TypeScript bindings
npm install @flight/memory-types
```

## Core V6R Use Cases

### 1. VM Memory Allocation Tracking

```typescript
import { 
  MemoryOperations, 
  MemorySize, 
  MemoryPurpose,
  MemoryAllocation 
} from '@flight/memory-types';

class V6RVMManager {
  private allocations = new Map<string, string[]>(); // sessionId -> allocationIds

  async createVM(sessionId: string, vmType: 'small' | 'medium' | 'large'): Promise<boolean> {
    const vmSize = this.getVMSize(vmType);
    
    try {
      const result = await MemoryOperations.createAllocation(
        sessionId,
        vmSize,
        MemoryPurpose.VmHeap
      );
      
      if (result.tag === 'ok') {
        // Track the allocation
        if (!this.allocations.has(sessionId)) {
          this.allocations.set(sessionId, []);
        }
        this.allocations.get(sessionId)!.push(result.val.id);
        
        console.log(`VM created: ${vmSize.humanReadable} for session ${sessionId}`);
        return true;
      } else {
        console.error('VM creation failed:', result.val.message);
        return false;
      }
    } catch (error) {
      console.error('VM creation error:', error);
      return false;
    }
  }

  private getVMSize(vmType: string): MemorySize {
    switch (vmType) {
      case 'small':
        return { bytes: 512n * 1024n * 1024n, humanReadable: "512MB" };
      case 'medium':
        return { bytes: 1024n * 1024n * 1024n, humanReadable: "1GB" };
      case 'large':
        return { bytes: 2048n * 1024n * 1024n, humanReadable: "2GB" };
      default:
        throw new Error(`Unknown VM type: ${vmType}`);
    }
  }

  async destroyVM(sessionId: string): Promise<boolean> {
    const allocationIds = this.allocations.get(sessionId);
    if (!allocationIds) {
      console.warn(`No VM found for session: ${sessionId}`);
      return false;
    }

    let allFreed = true;
    for (const allocationId of allocationIds) {
      const result = await MemoryOperations.freeAllocation(allocationId);
      if (result.tag === 'err') {
        console.error(`Failed to free allocation ${allocationId}:`, result.val.message);
        allFreed = false;
      }
    }

    this.allocations.delete(sessionId);
    console.log(`VM destroyed for session ${sessionId}`);
    return allFreed;
  }
}
```

### 2. Real-Time Memory Monitoring

```typescript
import { MemoryUsageSnapshot, MemoryPressure } from '@flight/memory-types';

class V6RMemoryMonitor {
  private websocket: WebSocket;
  private monitoringIntervals = new Map<string, NodeJS.Timeout>();

  constructor(websocket: WebSocket) {
    this.websocket = websocket;
  }

  async startMonitoring(sessionId: string, intervalMs: number = 5000): Promise<void> {
    // Get initial snapshot
    await this.updateMemoryStatus(sessionId);

    // Start periodic monitoring
    const interval = setInterval(() => {
      this.updateMemoryStatus(sessionId);
    }, intervalMs);

    this.monitoringIntervals.set(sessionId, interval);
    console.log(`Started memory monitoring for session ${sessionId}`);
  }

  stopMonitoring(sessionId: string): void {
    const interval = this.monitoringIntervals.get(sessionId);
    if (interval) {
      clearInterval(interval);
      this.monitoringIntervals.delete(sessionId);
      console.log(`Stopped memory monitoring for session ${sessionId}`);
    }
  }

  private async updateMemoryStatus(sessionId: string): Promise<void> {
    try {
      // Get memory snapshot
      const snapshotResult = await MemoryOperations.getMemorySnapshot(sessionId);
      if (snapshotResult.tag === 'err') {
        console.error('Failed to get memory snapshot:', snapshotResult.val.message);
        return;
      }

      // Get memory pressure
      const pressureResult = await MemoryOperations.getMemoryPressure(sessionId);
      if (pressureResult.tag === 'err') {
        console.error('Failed to get memory pressure:', pressureResult.val.message);
        return;
      }

      const snapshot = snapshotResult.val;
      const pressure = pressureResult.val;

      // Send update via WebSocket
      this.websocket.send(JSON.stringify({
        type: 'memory_update',
        sessionId,
        timestamp: snapshot.timestamp,
        memory: {
          used: snapshot.used.humanReadable,
          total: snapshot.total.humanReadable,
          available: snapshot.available.humanReadable,
          usagePercent: this.calculateUsagePercent(snapshot),
          fragmentation: Math.round(snapshot.fragmentationRatio * 100)
        },
        pressure,
        platform: snapshot.platform
      }));

      // Handle high memory pressure
      if (pressure === 'high' || pressure === 'critical') {
        await this.handleMemoryPressure(sessionId, pressure);
      }

    } catch (error) {
      console.error('Memory monitoring error:', error);
    }
  }

  private calculateUsagePercent(snapshot: MemoryUsageSnapshot): number {
    return Math.round(Number(snapshot.used.bytes) / Number(snapshot.total.bytes) * 100);
  }

  private async handleMemoryPressure(sessionId: string, pressure: MemoryPressure): Promise<void> {
    console.warn(`${pressure.toUpperCase()} memory pressure detected for session ${sessionId}`);
    
    // Notify frontend for user action
    this.websocket.send(JSON.stringify({
      type: 'memory_pressure_alert',
      sessionId,
      pressure,
      message: pressure === 'critical' 
        ? 'Critical memory usage - immediate action required'
        : 'High memory usage - consider scaling down'
    }));
  }
}
```

### 3. React Component Integration

```tsx
import React, { useState, useEffect } from 'react';
import { 
  MemoryUsageSnapshot, 
  MemoryOperations, 
  MemoryPressure 
} from '@flight/memory-types';

interface MemoryIndicatorProps {
  sessionId: string;
  refreshInterval?: number;
}

export const V6RMemoryIndicator: React.FC<MemoryIndicatorProps> = ({ 
  sessionId, 
  refreshInterval = 5000 
}) => {
  const [usage, setUsage] = useState<MemoryUsageSnapshot | null>(null);
  const [pressure, setPressure] = useState<MemoryPressure | null>(null);
  const [error, setError] = useState<string | null>(null);
  const [loading, setLoading] = useState(true);

  useEffect(() => {
    let isMounted = true;

    const updateMemory = async () => {
      try {
        const [snapshotResult, pressureResult] = await Promise.all([
          MemoryOperations.getMemorySnapshot(sessionId),
          MemoryOperations.getMemoryPressure(sessionId)
        ]);

        if (!isMounted) return;

        if (snapshotResult.tag === 'ok' && pressureResult.tag === 'ok') {
          setUsage(snapshotResult.val);
          setPressure(pressureResult.val);
          setError(null);
        } else {
          const errorMsg = snapshotResult.tag === 'err' 
            ? snapshotResult.val.message 
            : pressureResult.val.message;
          setError(errorMsg);
        }
      } catch (err) {
        if (isMounted) {
          setError(err instanceof Error ? err.message : 'Unknown error');
        }
      } finally {
        if (isMounted) {
          setLoading(false);
        }
      }
    };

    // Initial load
    updateMemory();

    // Set up interval
    const interval = setInterval(updateMemory, refreshInterval);

    return () => {
      isMounted = false;
      clearInterval(interval);
    };
  }, [sessionId, refreshInterval]);

  if (loading) {
    return (
      <div className="memory-indicator loading">
        <div className="spinner" />
        <span>Loading memory status...</span>
      </div>
    );
  }

  if (error) {
    return (
      <div className="memory-indicator error">
        <div className="error-icon">⚠️</div>
        <span>Memory Error: {error}</span>
      </div>
    );
  }

  if (!usage || !pressure) {
    return (
      <div className="memory-indicator no-data">
        <span>No memory data available</span>
      </div>
    );
  }

  const percentage = Number(usage.used.bytes) / Number(usage.total.bytes) * 100;
  const pressureColor = getPressureColor(pressure);
  const usageColor = getUsageColor(percentage);

  return (
    <div className="memory-indicator">
      <div className="memory-header">
        <h3>VM Memory Usage</h3>
        <span className={`pressure-badge ${pressureColor}`}>
          {pressure.toUpperCase()}
        </span>
      </div>

      <div className="memory-bar-container">
        <div className="memory-bar">
          <div 
            className={`memory-fill ${usageColor}`}
            style={{ width: `${percentage}%` }}
          />
        </div>
        <div className="memory-text">
          {usage.used.humanReadable} / {usage.total.humanReadable} 
          ({percentage.toFixed(1)}%)
        </div>
      </div>

      <div className="memory-details">
        <div className="detail-row">
          <span>Available:</span>
          <span>{usage.available.humanReadable}</span>
        </div>
        <div className="detail-row">
          <span>Platform:</span>
          <span>{usage.platform}</span>
        </div>
        {usage.fragmentationRatio > 0.3 && (
          <div className="detail-row warning">
            <span>Fragmentation:</span>
            <span>{(usage.fragmentationRatio * 100).toFixed(1)}%</span>
          </div>
        )}
      </div>

      {pressure === 'high' || pressure === 'critical' ? (
        <div className={`memory-alert ${pressureColor}`}>
          <strong>Memory Pressure Alert:</strong>
          <p>
            {pressure === 'critical' 
              ? 'Critical memory usage detected. Consider scaling down or restarting VM.'
              : 'High memory usage detected. Monitor closely and consider optimization.'
            }
          </p>
        </div>
      ) : null}
    </div>
  );
};

function getPressureColor(pressure: MemoryPressure): string {
  switch (pressure) {
    case 'low': return 'green';
    case 'medium': return 'yellow';
    case 'high': return 'orange';
    case 'critical': return 'red';
    default: return 'gray';
  }
}

function getUsageColor(percentage: number): string {
  if (percentage > 90) return 'red';
  if (percentage > 75) return 'orange';
  if (percentage > 50) return 'yellow';
  return 'green';
}
```

### 4. Memory Analytics Dashboard

```typescript
import { MemoryAnalytics, MemoryStats, MemoryTrend } from '@flight/memory-types';

class V6RMemoryDashboard {
  async generateMemoryReport(sessionId: string): Promise<void> {
    try {
      // Get comprehensive memory statistics
      const statsResult = await MemoryAnalytics.calculateMemoryStats(sessionId);
      if (statsResult.tag === 'err') {
        console.error('Failed to get memory stats:', statsResult.val.message);
        return;
      }

      // Get memory trends for the last 24 hours
      const trendsResult = await MemoryAnalytics.getMemoryTrends(
        sessionId, 
        24 * 60 * 60 // 24 hours in seconds
      );
      if (trendsResult.tag === 'err') {
        console.error('Failed to get memory trends:', trendsResult.val.message);
        return;
      }

      // Generate detailed report
      const reportResult = await MemoryAnalytics.generateMemoryReport(sessionId);
      if (reportResult.tag === 'err') {
        console.error('Failed to generate memory report:', reportResult.val.message);
        return;
      }

      const stats = statsResult.val;
      const trends = trendsResult.val;
      const report = reportResult.val;

      this.displayMemoryDashboard(stats, trends, report);

    } catch (error) {
      console.error('Memory dashboard error:', error);
    }
  }

  private displayMemoryDashboard(
    stats: MemoryStats, 
    trends: MemoryTrend, 
    report: string
  ): void {
    console.log('=== V6R Memory Dashboard ===');
    console.log(`Total Allocations: ${stats.totalAllocations}`);
    console.log(`Active Allocations: ${stats.activeAllocations}`);
    console.log(`Peak Memory: ${stats.peakMemory.humanReadable}`);
    console.log(`Current Memory: ${stats.currentMemory.humanReadable}`);
    console.log(`Average Allocation: ${stats.averageAllocationSize.humanReadable}`);
    console.log(`Efficiency Ratio: ${(stats.efficiencyRatio * 100).toFixed(1)}%`);
    console.log(`Memory Trend: ${trends.trendDirection}`);
    
    if (trends.predictedPeak) {
      console.log(`Predicted Peak: ${trends.predictedPeak.humanReadable}`);
    }

    console.log('\n=== Memory Usage by Purpose ===');
    stats.usageByPurpose.forEach(([purpose, size]) => {
      console.log(`${purpose}: ${size.humanReadable}`);
    });

    console.log('\n=== Detailed Report ===');
    console.log(report);
  }
}
```

## CSS Styles for React Components

```css
.memory-indicator {
  padding: 16px;
  border: 1px solid #e0e0e0;
  border-radius: 8px;
  background: white;
  margin: 16px 0;
}

.memory-indicator.loading {
  display: flex;
  align-items: center;
  gap: 8px;
}

.memory-indicator.error {
  background: #fef2f2;
  border-color: #fecaca;
  color: #dc2626;
  display: flex;
  align-items: center;
  gap: 8px;
}

.memory-header {
  display: flex;
  justify-content: space-between;
  align-items: center;
  margin-bottom: 12px;
}

.pressure-badge {
  padding: 4px 8px;
  border-radius: 4px;
  font-size: 12px;
  font-weight: bold;
}

.pressure-badge.green {
  background: #dcfce7;
  color: #166534;
}

.pressure-badge.yellow {
  background: #fef3c7;
  color: #92400e;
}

.pressure-badge.orange {
  background: #fed7aa;
  color: #c2410c;
}

.pressure-badge.red {
  background: #fecaca;
  color: #dc2626;
}

.memory-bar-container {
  margin-bottom: 12px;
}

.memory-bar {
  width: 100%;
  height: 20px;
  background: #f3f4f6;
  border-radius: 10px;
  overflow: hidden;
  margin-bottom: 4px;
}

.memory-fill {
  height: 100%;
  transition: width 0.3s ease;
}

.memory-fill.green {
  background: linear-gradient(90deg, #10b981, #059669);
}

.memory-fill.yellow {
  background: linear-gradient(90deg, #f59e0b, #d97706);
}

.memory-fill.orange {
  background: linear-gradient(90deg, #f97316, #ea580c);
}

.memory-fill.red {
  background: linear-gradient(90deg, #ef4444, #dc2626);
}

.memory-text {
  text-align: center;
  font-size: 14px;
  color: #6b7280;
}

.memory-details {
  border-top: 1px solid #e5e7eb;
  padding-top: 12px;
}

.detail-row {
  display: flex;
  justify-content: space-between;
  margin-bottom: 4px;
  font-size: 14px;
}

.detail-row.warning {
  color: #f59e0b;
  font-weight: 500;
}

.memory-alert {
  margin-top: 12px;
  padding: 12px;
  border-radius: 6px;
  border: 1px solid;
}

.memory-alert.orange {
  background: #fff7ed;
  border-color: #fed7aa;
  color: #c2410c;
}

.memory-alert.red {
  background: #fef2f2;
  border-color: #fecaca;
  color: #dc2626;
}

.spinner {
  width: 16px;
  height: 16px;
  border: 2px solid #e5e7eb;
  border-top: 2px solid #3b82f6;
  border-radius: 50%;
  animation: spin 1s linear infinite;
}

@keyframes spin {
  0% { transform: rotate(0deg); }
  100% { transform: rotate(360deg); }
}
```

## WebSocket Integration

```typescript
class V6RWebSocketManager {
  private ws: WebSocket;
  private memoryMonitor: V6RMemoryMonitor;

  constructor(url: string) {
    this.ws = new WebSocket(url);
    this.memoryMonitor = new V6RMemoryMonitor(this.ws);
    this.setupEventHandlers();
  }

  private setupEventHandlers(): void {
    this.ws.onopen = () => {
      console.log('WebSocket connected for memory monitoring');
    };

    this.ws.onmessage = (event) => {
      const message = JSON.parse(event.data);
      this.handleMessage(message);
    };

    this.ws.onerror = (error) => {
      console.error('WebSocket error:', error);
    };
  }

  private handleMessage(message: any): void {
    switch (message.type) {
      case 'start_monitoring':
        this.memoryMonitor.startMonitoring(message.sessionId);
        break;
      case 'stop_monitoring':
        this.memoryMonitor.stopMonitoring(message.sessionId);
        break;
      // Handle other message types...
    }
  }

  startMemoryMonitoring(sessionId: string): void {
    this.memoryMonitor.startMonitoring(sessionId);
  }

  stopMemoryMonitoring(sessionId: string): void {
    this.memoryMonitor.stopMonitoring(sessionId);
  }
}
```

This integration guide provides everything V6R needs to implement comprehensive VM memory management using the Flight memory types. The examples cover real-time monitoring, adaptive scaling, and user interface components for memory visualization.
